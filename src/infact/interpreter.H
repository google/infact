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
/// Provides an interpreter for assigning primitives and Factory-constructible
/// objects to named variables, as well as vectors thereof.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_INTERPRETER_H_
#define INFACT_INTERPRETER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "environment-impl.H"

namespace infact {

using std::iostream;
using std::ifstream;

class EnvironmentImpl;

/// Provides an interpreter for assigning primitives and
/// Factory-constructible objects to named variables, as well as
/// vectors thereof.  The interpreter maintains an internal
/// environment whereby previously defined variables may be used in
/// the definition of subsequent ones.  The syntax of this language
/// extends the syntax of the \link infact::Factory Factory \endlink
/// class, described in the documentation of the \link
/// infact::Factory::CreateOrDie Factory::CreateOrDie \endlink
/// method.
///
/// Statements in this language look like the following:
/// \code
/// // This is a comment.
/// bool b = true;    // assigns the value true to the boolean variable "b"
/// int f = 1;        // assigns the int value 1 to the variable "f"
/// double g = 2.4;   // assigns the double value 2.4 to the variable "g"
/// string n = "foo"  // assigns the string value "foo" to the variable "n"
/// bool[] b_vec = {true, false, true};  // assigns a vector of bool to "b_vec"
///
/// // Constructs an object of abstract type Model and assigns it to "m1"
/// Model m1 = PerceptronModel(name("foo"));
///
/// // Constructs an object of type Model and assigns it to "m2", crucially
/// // using the previously defined string variable "n" as the value for
/// // the PerceptronModel's name parameter.
/// Model m2 = PerceptronModel(name(n));
///
/// // Constructs a vector of Model objects and assigns it to "m_vec".
/// Model[] m_vec = {m2, PerceptronModel(name("bar"))};
/// \endcode
///
/// Additionally, the interpreter can do type inference, so all the type
/// specifiers in the previous examples are optional.  For example, one may
/// write the following statements:
/// \code
/// b = true;  // assigns the value true to the boolean variable "b"
///
/// // Constructs an object of abstract type Model and assigns it to "m1"
/// m1 = PerceptronModel(name("foo"));
///
/// // Constructs a vector of Model objects and assigns it to "m_vec".
/// m_vec = {m1, PerceptronModel(name("bar"))};
/// \endcode
///
/// Here's an example of using the interpreter after it has read the
/// three statements from the previous example from a file called
/// <tt>"example.infact"</tt>:
/// \code
/// #include "interpreter.H"
/// // ...
/// Interpreter i;
/// i.Eval("example.infact");
/// shared_ptr<Model> model;
/// vector<shared_ptr<Model> > model_vector;
/// bool b;
/// // The next statement only assigns a value to b if a variable "b"
/// // exists in the interpreter's environment.
/// i.Get("b", &b);
/// i.Get("m1", &model);
/// i.Get("m_vec", &model_vector);
/// \endcode
///
/// More formally, a statement in this language must conform to the
/// following grammar, defined on top of the BNF syntax in the
/// documentation of the \link infact::Factory::CreateOrDie
/// Factory::CreateOrDie \endlink method:
/// <table border=0>
/// <tr>
///   <td><tt>\<statement_list\></tt></td>
///   <td><tt>::=</tt></td>
///   <td><tt>[ \<statement\> ]*</tt></td>
/// </tr>
/// <tr>
///   <td><tt>\<statement\></tt></td>
///   <td><tt>::=</tt></td>
///   <td>
///     <tt>[ \<type_specifier\> ] \<variable_name\> '=' \<value\> ';' </tt>
///   </td>
/// </tr>
/// <tr>
///   <td valign=top><tt>\<type_specifier\></tt></td>
///   <td valign=top><tt>::=</tt></td>
///   <td valign=top>
///     <table border="0">
///       <tr><td><tt>"bool" | "int" | "string" | "double" | "bool[]" | "int[]"
///                   "string[]" | "double[]" | T | T[]</tt></td></tr>
///       <tr><td>where <tt>T</tt> is any \link infact::Factory
///               Factory\endlink-constructible type.</td></tr>
///     </table>
///   </td>
/// </tr>
/// <tr>
///   <td><tt>\<variable_name\></tt></td>
///   <td><tt>::=</tt></td>
///   <td>any valid C++ identifier</td>
/// </tr>
/// <tr>
///   <td valign=top><tt>\<value\></tt></td>
///   <td valign=top><tt>::=</tt></td>
///   <td valign=top><tt>\<literal\> | '{' \<literal_list\> '}' |<br>
///                      \<spec\> | '{' \<spec_list\> '}'</tt>
///   </td>
/// </tr>
/// </table>
///
/// The above grammar doesn&rsquo;t contain rules covering C++ style
/// line comments, but they have the same behavior in this language as
/// they do in C++, i.e., everything after the <tt>//</tt> to the end
/// of the current line is treated as a comment and ignored.  There
/// are no C-style comments in this language.
class Interpreter {
 public:
  /// Constructs a new instance with the specified debug level.  The
  /// wrapped \link infact::Environment Environment \endlink will
  /// also have the specified debug level.
  Interpreter(int debug = 0) {
    env_ = new EnvironmentImpl(debug);
  }

  /// Destroys this interpreter.
  virtual ~Interpreter() {
    delete env_;
  }

  /// Evaluates the statements in the specified text file.
  void Eval(const string &filename) {
    filename_ = filename;
    ifstream file(filename_.c_str());
    Eval(file);
  }

  /// Evaluates the statements in the specified string.
  void EvalString(const string& input) {
    StreamTokenizer st(input);
    Eval(st);
  }

  /// Evaluates the statements in the specified stream.
  void Eval(istream &is) {
    StreamTokenizer st(is);
    Eval(st);
  }


  void PrintEnv(ostream &os) const {
    env_->Print(os);
  }

  void PrintFactories(ostream &os) const {
    env_->PrintFactories(os);
  }

  /// Retrieves the value of the specified variable.  It is an error
  /// if the type of the specified pointer to a value object is different
  /// from the specified variable in this interpreter&rsquo;s environment.
  ///
  /// \tparam the type of value object being set by this method
  ///
  /// \param varname the name of the variable for which to retrieve the value
  /// \param value   a pointer to the object whose value to be set by this
  ///                method
  template<typename T>
  bool Get(const string &varname, T *value) const {
    return env_->Get(varname, value);
  }

  /// Returns a pointer to the environment of this interpreter.
  /// Crucially, this method returns a pointer to the Environment
  /// implementation class, \link infact::EnvironmentImpl
  /// EnvironmentImpl\endlink, so that its templated \link
  /// infact::EnvironmentImpl::Get EnvironmentImpl::Get \endlink
  /// method may be invoked.
  EnvironmentImpl *env() { return env_; }

 private:
  /// Evalutes the expressions contained in the specified token stream.
  void Eval(StreamTokenizer &st);

  void WrongTokenError(size_t pos,
                       const string &expected,
                       const string &found,
                       StreamTokenizer::TokenType found_type) const;

  void WrongTokenTypeError(size_t pos,
                           StreamTokenizer::TokenType expected,
                           StreamTokenizer::TokenType found,
                           const string &token) const;

  void WrongTokenTypeError(size_t pos,
                           const string &expected_type,
                           const string &found_type,
                           const string &token) const;

  /// The environment of this interpreter.
  EnvironmentImpl *env_;

  /// The name of the file being interpreted, or the empty string if there
  /// is no file associated with the stream being interpreted.
  string filename_;
};

}  // namespace infact

#endif
