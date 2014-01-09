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
/// Provides a generic dynamic object factory.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_STREAM_INIT_H_
#define INFACT_STREAM_INIT_H_

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>

#include "stream-tokenizer.H"

namespace infact {

using std::ostringstream;
using std::shared_ptr;
using std::unordered_map;
using std::vector;

class Environment;

/// \class StreamInitializer
///
/// An interface that allows for a primitive, \link infact::Factory
/// Factory\endlink-constructible object or vector thereof to be
/// initialized based on the next token or tokens from a token stream.
/// The data member may be an <tt>int</tt>, a <tt>double</tt>, a
/// <tt>string</tt> or a <tt>shared_ptr</tt> to another
/// Factory-constructible type.
class StreamInitializer {
 public:
  StreamInitializer() { }
  virtual ~StreamInitializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) = 0;
};

template <typename T> class Factory;

/// \class Initializer
///
/// A class to initialize a \link Factory\endlink-constructible
/// object.
///
/// \tparam T a <tt>shared_ptr</tt> to any type constructible by a Factory
template <typename T>
class Initializer : public StreamInitializer {
 public:
  Initializer(T *member) : member_(member) { }
  virtual ~Initializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) {
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    bool is_null =
      token_type == StreamTokenizer::RESERVED_WORD &&
      (st.Peek() == "nullptr" || st.Peek() == "nullptr");
    if (!(is_null || token_type == StreamTokenizer::IDENTIFIER)) {
      ostringstream err_ss;
      err_ss << "FactoryInitializer: expected \"nullptr\", \"nullptr\" or "
	     << "IDENTIFIER token at stream "
             << "position " << st.PeekTokenStart() << " but found "
             << StreamTokenizer::TypeName(token_type) << " token: \""
             << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    Factory<typename T::element_type> factory;
    (*member_) = factory.CreateOrDie(st, env);
  }
 private:
  T *member_;
};

/// A specialization to allow Factory-constructible objects to initialize
/// <tt>int</tt> data members.
template<>
class Initializer<int> : public StreamInitializer {
 public:
  Initializer(int *member) : member_(member) { }
  virtual ~Initializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) {
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    if (token_type != StreamTokenizer::NUMBER) {
      ostringstream err_ss;
      err_ss << "IntInitializer: expected NUMBER token at stream "
             << "position " << st.PeekTokenStart() << " but found "
             << StreamTokenizer::TypeName(token_type) << " token: \""
             << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    (*member_) = atoi(st.Next().c_str());
  }
 private:
  int *member_;
};

/// A specialization to initialize <tt>double</tt> data members.
template<>
class Initializer<double> : public StreamInitializer {
 public:
  Initializer(double *member) : member_(member) { }
  virtual ~Initializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) {
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    if (token_type != StreamTokenizer::NUMBER) {
      ostringstream err_ss;
      err_ss << "DoubleInitializer: expected NUMBER token at stream "
             << "position " << st.PeekTokenStart() << " but found "
             << StreamTokenizer::TypeName(token_type) << " token: \""
             << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    (*member_) = atof(st.Next().c_str());
  }
 private:
  double *member_;
};

/// A specialization to initialize <tt>bool</tt> data members.
template<>
class Initializer<bool> : public StreamInitializer {
 public:
  Initializer(bool *member) : member_(member) { }
  virtual ~Initializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) {
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    if (token_type != StreamTokenizer::RESERVED_WORD) {
      ostringstream err_ss;
      err_ss << "BoolInitializer: expected RESERVED_WORD token at stream "
             << "position " << st.PeekTokenStart() << " but found "
             << StreamTokenizer::TypeName(token_type) << " token: \""
             << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    size_t next_tok_start = st.PeekTokenStart();
    string next_tok = st.Next();
    if (next_tok == "false") {
      (*member_) = false;
    } else if (next_tok == "true") {
      (*member_) = true;
    } else {
      ostringstream err_ss;
      err_ss << "Initializer<bool>: expected either \"true\" or \"false\" "
             << "token at stream position " << next_tok_start << " but found "
             << "token: \"" << next_tok << "\"";
      throw new std::runtime_error(err_ss.str());
    }
  }
 private:
  bool *member_;
};

/// A specialization to initialize <tt>string</tt> data members.
template<>
class Initializer<string> : public StreamInitializer {
 public:
  Initializer(string *member) : member_(member) { }
  virtual ~Initializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env = nullptr) {
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    if (token_type != StreamTokenizer::STRING) {
      ostringstream err_ss;
      err_ss << "StringInitializer: expected STRING token at stream "
             << "position " << st.PeekTokenStart() << " but found "
             << StreamTokenizer::TypeName(token_type) << " token: \""
             << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    (*member_) = st.Next();
  }
 private:
  string *member_;
};

}  // namespace infact

#endif
