// Copyright 2014, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following disclaimer
//     in the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Google Inc. nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -----------------------------------------------------------------------------
//
//
/// \file
/// Interpreter implementation.
/// Author: dbikel@google.com (Dan Bikel)

#include <sstream>

#include "error.h"
#include "interpreter.h"

using namespace std;

namespace infact {

using std::ostringstream;

// TODO(dbikel): Use std::make_unique here when we start using C++14.
unique_ptr<istream>
DefaultIStreamBuilder::Build(const string &filename,
                             std::ios_base::openmode mode) const {
  return unique_ptr<istream>(new std::ifstream(filename, mode));
}

bool
Interpreter::IsAbsolute(const string &filename) const {
  return filename.length() > 0 && filename[0] == '/';
}

bool
Interpreter::CanReadFile(const string &filename) const {
  unique_ptr<istream> file = std::move(istream_builder_->Build(filename));
  return file->good();
}

bool
Interpreter::CanReadFile(const string &f1, const string &f2,
                         string *filename) const {
  if (CanReadFile(f1)) {
    *filename = f1;
    return true;
  } else if (CanReadFile(f2)) {
    *filename = f2;
    return true;
  } else {
    return false;
  }
}

bool
Interpreter::HasCycle(const string &filename,
                      const vector<string> &filenames) const {
  for (const string &f : filenames) {
    if (filename == f) {
      return true;
    }
  }
  return false;
}

void
Interpreter::Eval(const string &filename) {
  if (!CanReadFile(filename)) {
    ostringstream err_ss;
    err_ss << "infact::Interpreter: error: cannot read file "
           << "\"" << filename << "\" (or file does not exist)\n";
    Error(err_ss.str());
  }
  EvalFile(filename);
}

// This is the implementation of the private EvalFile method, used
// both the top-level Eval method above, as well as for importing
// files.
void
Interpreter::EvalFile(const string &filename) {
  filenames_.push_back(filename);
  unique_ptr<istream> file =
      std::move(istream_builder_->Build(curr_filename()));
  Eval(*file);
  filenames_.pop_back();
}

void
Interpreter::Import(StreamTokenizer &st) {
  // Consume reserved word "import".
  st.Next();

  if (st.PeekTokenType() != StreamTokenizer::STRING) {
    string expected_type =
        string(StreamTokenizer::TypeName(StreamTokenizer::RESERVED_WORD));
    string found_type = StreamTokenizer::TypeName(st.PeekTokenType());
    WrongTokenTypeError(st, st.PeekTokenStart(), expected_type, found_type,
                        st.Peek());
  }

  if (debug_ >= 1) {
    std::cerr << "infact::Interpreter: from file \"" << curr_filename()
              << "\" importing \"" << st.Peek() << "\"\n";
  }

  // Grab the string naming the file to be imported.
  string original_import_filename = st.Next();
  string relative_import_filename = original_import_filename;

  // Test to see if the named file exists.  If the path is not
  // absolute, we try to get the dirname of current file, if it
  // exists, and create a relative path, which takes precedence
  // over a path relative to the current working directory.
  bool relative = false;
  if (!IsAbsolute(original_import_filename)) {
    size_t slash_pos = curr_filename().rfind('/');
    if (slash_pos != string::npos) {
      string dirname = curr_filename().substr(0, slash_pos);
      relative_import_filename = dirname + '/' + original_import_filename;
      relative = true;
    }
  }

  string import_filename;
  if (!CanReadFile(relative_import_filename, original_import_filename,
                   &import_filename)) {
    ostringstream err_ss;
    err_ss << "infact::Interpreter: " << filestack(st, st.tellg())
           << "error: cannot read file \"";
    if (relative) {
      err_ss << relative_import_filename << "\" or \"";
    }
    err_ss << original_import_filename << "\" (or file does not exist)\n";
    Error(err_ss.str());
  } else {
    if (debug_ >= 1) {
      std::cerr << "infact::Interpreter: tested paths \""
                << relative_import_filename << "\" and \""
                << original_import_filename << "\" and found that \""
                << import_filename << "\" exists and is readable\n";
    }
  }

  if (HasCycle(import_filename, filenames_)) {
    ostringstream err_ss;
    err_ss << "infact::Interpreter: " << filestack(st, st.tellg())
           << "attempted import of file \"" << import_filename << "\" "
           << "from file \"" << curr_filename() << "\" introduces cycle";
    Error(err_ss.str());
  }

  // Finally, evaluate file using the private EvalFile method.  The imported
  // file gets interpreted using the current Environment.
  EvalFile(import_filename);

  if (st.Peek() != ";") {
    WrongTokenError(st, st.PeekTokenStart(), ";", st.Peek(),
                    st.PeekTokenType());
  }

  // Consume semicolon.
  st.Next();
}

void
Interpreter::Eval(StreamTokenizer &st) {
  // Keeps reading import or assignment statements until there are no
  // more tokens.
  while (st.PeekTokenType() != StreamTokenizer::EOF_TYPE) {
#ifdef INFACT_THROW_EXCEPTIONS
    try {
#endif
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    // First, see if we have an import statement.
    if (token_type == StreamTokenizer::RESERVED_WORD && st.Peek() == "import") {
      Import(st);
      // Now continue this loop reading either assignment or import statements.
      continue;
    }
    // Read variable name or type specifier.
    VarMapBase *varmap = env_->GetVarMapForType(st.Peek());
    bool is_type_specifier =  varmap != nullptr;
    if (token_type != StreamTokenizer::IDENTIFIER && !is_type_specifier) {
      string expected_type =
          string(StreamTokenizer::TypeName(StreamTokenizer::IDENTIFIER)) +
          " or type specifier";
      string found_type = StreamTokenizer::TypeName(token_type);
      WrongTokenTypeError(st, st.PeekTokenStart(), expected_type, found_type,
                          st.Peek());
    }

    string type = "";
    if (is_type_specifier) {
      // Consume and remember the type specifier.
      st.Next();              // Explicit type could be a concrete type.
      type = varmap->Name();  // Remember the abstract type.

      // Check that next token is a variable name.
      StreamTokenizer::TokenType token_type = st.PeekTokenType();
      if (token_type != StreamTokenizer::IDENTIFIER) {
        WrongTokenTypeError(st, st.PeekTokenStart(),
                            StreamTokenizer::IDENTIFIER, token_type, st.Peek());
      }
    }

    string varname = st.Next();

    // Next, read equals sign.
    token_type = st.PeekTokenType();
    if (st.Peek() != "=") {
      WrongTokenError(st, st.PeekTokenStart(), "=", st.Peek(),
                      st.PeekTokenType());
    }

    // Consume equals sign.
    st.Next();

    if (st.PeekTokenType() == StreamTokenizer::EOF_TYPE) {
      ostringstream err_ss;
      err_ss << "infact::Interpreter: " << filestack(st, st.tellg())
             << "error: unexpected EOF";
      Error(err_ss.str());
    }

    // Consume and set the value for this variable in the environment.
    env_->ReadAndSet(varname, st, type);

    token_type = st.PeekTokenType();
    if (st.Peek() != ";") {
      WrongTokenError(st, st.PeekTokenStart(), ";", st.Peek(),
                      st.PeekTokenType());
    }
    // Consume semicolon.
    st.Next();
#ifdef INFACT_THROW_EXCEPTIONS
    }
    catch (std::runtime_error &e) {
      cerr << "infact::Interpreter: caught exception\n"
           << filestack(st, st.tellg())
           << "==================\n"
           << "Exception message:\n"
           << "==================\n" << e.what() << "\n"
           << endl;
      // For now, we simply give up.
      break;
    }
#endif
  }
}

string
Interpreter::filestack(StreamTokenizer &st, size_t pos) const {
  ostringstream message;
  size_t line_pos = (st.PeekPrevTokenLineStart() <= pos ?
                     pos - st.PeekPrevTokenLineStart() : pos);
  message << "in file \"" << curr_filename() << "\" "
          << "(line: " << st.PeekPrevTokenLineNumber() + 1
          << "; line pos: " << line_pos << "; stream pos: " << pos << "):\n";
  auto it = filenames_.rbegin();
  if (it != filenames_.rend()) {
    ++it;
  }
  for (; it != filenames_.rend(); ++it) {
    const string &filename = *it;
    message << "\timported from \"" << filename << "\"\n";
  }
  string line = st.line();
  message << "here:\n"
          << line << "\n";
  if (st.PeekPrevTokenLineStart() <= pos) {
    for (int i = line_pos; i > 0; --i) {
      message << " ";
    }
  }
  message << "^\n";
  return message.str();
}

void
Interpreter::WrongTokenError(StreamTokenizer &st,
                             size_t pos,
                             const string &expected,
                             const string &found,
                             StreamTokenizer::TokenType found_type) const {
  // If possible, consume the wrong token.
  if (st.HasNext()) {
    st.Next();
  }
  ostringstream err_ss;
  err_ss << "infact::Interpreter: " << filestack(st, pos)
         << "expected token \"" << expected << "\" but found \"" << found
         << "\" (token type: " << StreamTokenizer::TypeName(found_type)
         << ")";
  Error(err_ss.str());
}

void
Interpreter::WrongTokenTypeError(StreamTokenizer &st,
                                 size_t pos,
                                 StreamTokenizer::TokenType expected,
                                 StreamTokenizer::TokenType found,
                                 const string &token) const {
  WrongTokenTypeError(st, pos,
                      StreamTokenizer::TypeName(expected),
                      StreamTokenizer::TypeName(found),
                      token);
}

void
Interpreter::WrongTokenTypeError(StreamTokenizer &st,
                                 size_t pos,
                                 const string &expected_type,
                                 const string &found_type,
                                 const string &token) const {
  // If possible, consume the wrong token.
  if (st.HasNext()) {
    st.Next();
  }
  ostringstream err_ss;
  err_ss << "infact::Interpreter: " << filestack(st, pos)
         << "expected token type " << expected_type
         << " but found " << found_type
         << "; token=\"" << token << "\"";
  Error(err_ss.str());
}

}  // namespace infact
