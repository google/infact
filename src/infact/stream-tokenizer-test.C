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
/// Test driver for the \link infact::StreamTokenizer StreamTokenizer \endlink
/// class.
/// \author dbikel@google.com (Dan Bikel)

#include <iostream>

#include "stream-tokenizer.H"

using namespace infact;
using namespace std;

int
main(int argc, char **argv) {
  cerr << "Testing StreamTokenizer with string arg constructor:" << endl;
  string test_string = "foo(bar, baz(34), bleh(\"fleh with spaces\"))";
  StreamTokenizer st1(test_string);
  while (st1.HasNext()) {
    StreamTokenizer::TokenType type = st1.PeekTokenType();
    size_t start = st1.PeekTokenStart();
    size_t line_number = st1.PeekTokenLineNumber();
    cout << "token: \"" << st1.Next() << "\""
         << "; type=" << StreamTokenizer::TypeName(type)
	 << "; start=" << start
	 << "; line=" << line_number
	 << endl;
    cout << "chars so far: '" << st1.str() << "'" << endl;
  }
  cerr << "Done." << endl;

  cerr << "\nTesting Putback:" << endl;
  st1.Putback();
  while (st1.HasNext()) {
    StreamTokenizer::TokenType type = st1.PeekTokenType();
    size_t start = st1.PeekTokenStart();
    size_t line_number = st1.PeekTokenLineNumber();
    cout << "token: \"" << st1.Next() << "\""
         << "; type=" << StreamTokenizer::TypeName(type)
	 << "; start=" << start
	 << "; line=" << line_number
	 << endl;
    cout << "chars so far: '" << st1.str() << "'" << endl;
  }

  cerr << "\nTesting Rewind(3):" << endl;
  st1.Rewind(3);
  while (st1.HasNext()) {
    StreamTokenizer::TokenType type = st1.PeekTokenType();
    size_t start = st1.PeekTokenStart();
    size_t line_number = st1.PeekTokenLineNumber();
    cout << "token: \"" << st1.Next() << "\""
         << "; type=" << StreamTokenizer::TypeName(type)
	 << "; start=" << start
	 << "; line=" << line_number
	 << endl;
    cout << "chars so far: '" << st1.str() << "'" << endl;
  }

  cerr << "Testing Rewind():" << endl;
  st1.Rewind();
  while (st1.HasNext()) {
    StreamTokenizer::TokenType type = st1.PeekTokenType();
    size_t start = st1.PeekTokenStart();
    size_t line_number = st1.PeekTokenLineNumber();
    cout << "token: \"" << st1.Next() << "\""
         << "; type=" << StreamTokenizer::TypeName(type)
	 << "; start=" << start
	 << "; line=" << line_number
	 << endl;
    cout << "chars so far: '" << st1.str() << "'" << endl;
  }

  cerr << "\nReading from stdin until EOF:" << endl;

  StreamTokenizer st2(cin);
  while (st2.HasNext()) {
    StreamTokenizer::TokenType type = st2.PeekTokenType();
    cout << "token: \"" << st2.Next() << "\""
         << "; type=" << StreamTokenizer::TypeName(type) << endl;
  }
}
