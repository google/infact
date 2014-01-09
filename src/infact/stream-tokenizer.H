// Copyright 2012, Google Inc.
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
/// Provides the \link infact::StreamTokenizer StreamTokenizer \endlink class.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_STREAM_TOKENIZER_H_
#define INFACT_STREAM_TOKENIZER_H_

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string.h>
#include <vector>

namespace infact {

using std::istream;
using std::istringstream;
using std::ostringstream;
using std::set;
using std::streampos;
using std::string;
using std::vector;
using std::cerr;
using std::endl;

/// Default set of reserved words for the StreamTokenizer class.
/// Use the \link infact::StreamTokenizer::set_reserved_words
/// StreamTokenizer::set_reserved_words \endlink
/// to customize this set.
static const char *default_reserved_words[] = {
  "-",
  "nullptr",
  "NULL",
  "false",
  "true",
  "bool",
  "int",
  "double",
  "string",
  "bool[]",
  "int[]",
  "double[]",
  "string[]",
};

/// Default set of reserved characters for the StreamTokenizer class.
#define DEFAULT_RESERVED_CHARS "(){},=;/"

/// \class StreamTokenizer
///
/// A simple class for tokenizing a stream of tokens for the formally
/// specified language used to construct objects for the InFact
/// framework.
class StreamTokenizer {
 public:
  /// The set of types of tokens read by this stream tokenizer.
  ///
  /// \see PeekTokenType
  enum TokenType {
    EOF_TYPE,
    RESERVED_CHAR,
    RESERVED_WORD,
    STRING,
    NUMBER,
    IDENTIFIER
  };

  /// Returns a string type name for the specified TokenType constant.
  static const char *TypeName(TokenType token_type) {
    static const char *names[] = {
      "EOF", "RESERVED_CHAR", "RESERVED_WORD", "STRING", "NUMBER", "IDENTIFIER"
    };
    return names[token_type];
  }

  /// Information about a token read from the underlying stream.
  struct Token {
    /// The token itself.
    string tok;
    /// The token&rsquo;s type.
    TokenType type;

    // The following three fields capture information about the underlying
    // byte stream at the time this token was read from it.

    /// The starting byte of the token in the underlying stream.
    size_t start;
    /// The line number of the first byte of the token in the underlying stream.
    size_t line_number;
    /// The current position in the underlying stream just after reading this
    /// token.
    size_t curr_pos;
  };

  /// Constructs a new instance around the specified byte stream.
  ///
  /// \param is             the input byte stream for this stream tokenizer to
  ///                       use
  /// \param reserved_chars the set of single characters serving as
  ///                       &ldquo;reserved characters&rdquo;
  StreamTokenizer(istream &is,
                  const char *reserved_chars = DEFAULT_RESERVED_CHARS) :
      is_(is), num_read_(0), line_number_(0), eof_reached_(false),
      next_token_idx_(0) {
    Init(reserved_chars);
  }

  /// Constructs a new instance around the specified string.
  ///
  /// \param s              the string providing the stream of characters
  ///                       for this stream tokenizer to use
  /// \param reserved_chars the set of single characters serving as
  ///                       &ldquo;reserved characters&rdquo;
  StreamTokenizer(const string &s,
                  const char *reserved_chars = DEFAULT_RESERVED_CHARS) :
      sstream_(s), is_(sstream_), num_read_(0), line_number_(0),
      eof_reached_(false), next_token_idx_(0) {
    Init(reserved_chars);
  }

  /// Sets the set of &ldquo;reserved words&rdquo; used by this stream
  /// tokenizer.  Should be invoked just after construction time.
  void set_reserved_words(set<string> &reserved_words) {
    reserved_words_ = reserved_words;
  }

  /// Destroys this instance.
  virtual ~StreamTokenizer() {
    delete[] reserved_chars_;
  }

  /// Returns the entire sequence of characters read so far by this
  /// stream tokenizer as a newly constructed string object.
  string str() { return oss_.str(); }

  /// Returns the number of bytes read from the underlying byte
  /// stream just after scanning the most recent token, or 0 if this stream
  /// is just about to return the first token.
  size_t tellg() const {
    return HasPrev() ? token_[next_token_idx_ - 1].curr_pos : 0;
  }

  /// Returns the number of lines read from the underlying byte stream,
  /// where a line is any number of bytes followed by a newline character
  /// (i.e., this is ASCII-centric).
  size_t line_number() const {
    return HasNext() ? token_[next_token_idx_].line_number : line_number_;
  }

  /// Returns whether there is another token in the token stream.
  bool HasNext() const { return next_token_idx_ < token_.size(); }

  bool HasPrev() const { return next_token_idx_ > 0; }

  string PeekPrev() const {
    return HasPrev() ? token_[next_token_idx_ - 1].tok : "";
  }

  size_t PeekPrevTokenStart() const {
    return HasPrev() ? token_[next_token_idx_ - 1].start : 0;
  }

  TokenType PeekPrevTokenType() const {
    return HasPrev() ? token_[next_token_idx_ - 1].type : EOF_TYPE;
  }

  /// Returns the next token in the token stream.
  string Next() {
    if (!HasNext()) {
      // Error.
      throw std::runtime_error("invoking StreamTokenizer::Next when HasNext "
			       "returns false");
    }

    size_t curr_token_idx = next_token_idx_;

    // Try to get the next token of the stream if we're about to run out of
    // tokens.
    if (!eof_reached_ && next_token_idx_ + 1 == token_.size()) {
      Token next;
      if (GetNext(&next)) {
	token_.push_back(next);
      }
    }
    // Ensure that we only advance if we haven't already reached token_.size().
    if (next_token_idx_ < token_.size()) {
      ++next_token_idx_;
    }

    return token_[curr_token_idx].tok;
  }

  /// Rewinds this token stream to the beginning.  If the underlying stream
  /// has no tokens, this is a no-op.
  void Rewind() {
    next_token_idx_ = 0;
  }

  /// Rewinds this token stream by the specified number of tokens.  If the
  /// specified number of tokens is greater than the number of tokens read
  /// so far, invoking this method will be functionally equivalent to invoking
  /// the no-argument Rewind() method.
  void Rewind(size_t num_tokens) {
    // Cannot rewind more than the number of tokens read so far.
    if (num_tokens > next_token_idx_) {
      num_tokens = next_token_idx_;
    }
    next_token_idx_ -= num_tokens;
  }

  /// A synonym for Rewind(1).
  void Putback() {
    Rewind(1);
  }

  /// Returns the next token&rsquo;s start position, or the byte position
  /// of the underlying byte stream if there is no next token.
  size_t PeekTokenStart() const {
    return HasNext() ? token_[next_token_idx_].start : num_read_;
  }

  /// Returns the type of the next token, or EOF_TYPE if there is no next
  /// token.
  TokenType PeekTokenType() const {
    return HasNext() ? token_[next_token_idx_].type : EOF_TYPE;
  }

  /// Returns the line number of the first byte of the next token, or
  /// the current line number of the underlying stream if there is no
  /// next token.
  size_t PeekTokenLineNumber() const {
    return HasNext() ? token_[next_token_idx_].line_number : line_number_;
  }

  /// Returns the next token that would be returned by the \link Next
  /// \endlink method.  The return value of this method is only valid
  /// when \link HasNext \endlink returns <tt>true</tt>.
  string Peek() const { return HasNext() ? token_[next_token_idx_].tok : ""; }

 private:
  void Init(const char *reserved_chars) {
    num_reserved_chars_ = strlen(reserved_chars);
    reserved_chars_ = new char[num_reserved_chars_];
    strcpy(reserved_chars_, reserved_chars);
    int num_reserved_words = sizeof(default_reserved_words)/sizeof(const char*);
    for (int i = 0; i < num_reserved_words; ++i) {
      reserved_words_.insert(string(default_reserved_words[i]));
    }
    Token next;
    if (GetNext(&next)) {
      token_.push_back(next);
    }
  }

  void ConsumeChar(char c);

  bool ReadChar(char *c);

  /// Retrieves the next token from the <tt>istream</tt> wrapped by this
  /// stream tokenizer.
  ///
  /// \param next the token object to be filled in if there is a next token
  ///
  /// \return whether there was a next token in the underlying stream and
  ///         it was successfully gotten
  bool GetNext(Token *next);

  /// Returns whether the specified character represents a
  /// &ldquo;reserved character&rdquo;.
  bool ReservedChar(char c) const {
    for (size_t i = 0; i < num_reserved_chars_; ++i) {
      if (c == reserved_chars_[i]) {
        return true;
      }
    }
    return false;
  }

  // data members

  // The stream itself.
  /// This data member is for use when we need to construct the is_ data member
  /// from a string at construction time.
  istringstream sstream_;
  /// The underlying byte stream of this token stream.
  istream &is_;

  // Information about special tokens.
  char *reserved_chars_;
  size_t num_reserved_chars_;
  set<string> reserved_words_;

  // Information about the current state of the underlying byte stream.
  size_t num_read_;
  size_t line_number_;
  bool eof_reached_;
  ostringstream oss_;

  // The sequence of tokens read so far.
  vector<Token> token_;

  // The index of the next token in this stream in token_, or token_.size()
  // if there are no more tokens left in this stream.  Note that invocations
  // of the Rewind and Putback methods alter this data member.
  size_t next_token_idx_;
};

}  // namespace infact

#endif
