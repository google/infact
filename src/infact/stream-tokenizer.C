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
/// Implementation of the \link infact::StreamTokenizer StreamTokenizer
/// \endlink class.
/// \author dbikel@google.com (Dan Bikel)

#include <ctype.h>
#include <stdexcept>

#include "stream-tokenizer.H"

namespace infact {

void
StreamTokenizer::ConsumeChar(char c) {
  oss_ << c;
  ++num_read_;
  if (c == '\n') {
    ++line_number_;
  }
}

bool
StreamTokenizer::ReadChar(char *c) {
  (*c) = is_.get();
  if (!is_.good()) {
    eof_reached_ = true;
    return false;
  } else {
    ConsumeChar(*c);
    return true;
  }
}

bool
StreamTokenizer::GetNext(Token *next) {
  if (!is_.good()) {
    eof_reached_ = true;
    return false;
  }
  // Get first character of next token.
  char c;
  bool is_whitespace = true;
  while (is_whitespace) {
    if (!ReadChar(&c)) {
      return false;
    }
    is_whitespace = isspace(c);

    // If we find a comment character, then read to the end of the line.
    if (!is_whitespace && c == '/' && is_.peek() == '/') {
      while (c != '\n') {
        if (!ReadChar(&c)) {
          return false;
        }
      }
      is_whitespace = true;
    }
  }

  // In case we're successful in reading the next token, we fill in
  // the two stream state data available now.
  next->start = num_read_ - 1;
  next->line_number = line_number_;

  bool next_tok_complete = false;
  next->tok.clear();
  if (ReservedChar(c)) {
    next->tok += c;
    next_tok_complete = true;
    next->type = RESERVED_CHAR;
  } else if (c == '"') {
    // We've got a string literal, so keep reading characters,
    // until hitting a non-escaped double quote.
    streampos string_literal_start_pos = num_read_ - 1;
    bool found_closing_quote = false;
    while (is_.good()) {
      bool success = ReadChar(&c);
      if (success) {
        if (c == '"') {
          found_closing_quote = true;
          break;
        } else if (c == '\\') {
          success = ReadChar(&c);
        }
      }
      if (success) {
        next->tok += c;
      }
    }
    if (!found_closing_quote) {
      cerr << "StreamTokenizer: error: could not find closing "
           << "double quote for string literal beginning at stream index "
           << string_literal_start_pos
           << "; partial string literal read: \""
           << next->tok << endl;
      throw std::runtime_error("unclosed string literal");
    }
    next_tok_complete = true;
    next->type = STRING;
  } else {
    // This is a number, a reserved word or C++ identifier token, so
    // add first character; the remainder of the token will be handled
    // in the next block.
    next->tok += c;
    next->type = (c == '-' || (c >= '0' && c <= '9')) ? NUMBER : IDENTIFIER;
  }
  if (!next_tok_complete) {
    // The current token is a number, a reserved word or C++
    // identifier, so we keep reading characters until hitting a
    // "reserved character", a whitespace character or EOF.
    bool done = false;
    while (!done && is_.good()) {
      // We don't call ReadChar below because the next character might
      // tell us that the current token has ended (i.e., if it's a
      // reserved character, a double quote or a whitespace
      // character).
      int peek = is_.peek();
      if (peek != EOF) {
        char next_char = static_cast<char>(peek);
        if (ReservedChar(next_char) || next_char == '"' || isspace(next_char)) {
          // Now that we've finished reading something that is not a
          // string literal, change its type to be RESERVED_WORD if it
          // exactly matches something in the set of reserved words.
          if (reserved_words_.count(next->tok) != 0) {
            next->type = RESERVED_WORD;
          }
          done = true;
        } else {
          ReadChar(&c);
          next->tok += c;
        }
      } else {
	eof_reached_ = true;
      }
    }
  }
  // We're about to return a successfully read token, so we make sure to record
  // the stream position at this point in the Token object.
  next->curr_pos = num_read_;

  return true;
}

}  // namespace infact
