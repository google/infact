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
/// Provides implementations of the various example classes.

#include "example.H"
#include "environment-impl.H"

namespace infact {

// Normally, the various IMPLEMENT_FACTORY declarations would be
// separate from the various REGISTER_* declarations, which would
// usually appear in the separate concrete implementations' .C files.
// For compactness, we have this single .C file, so everything is
// lumped together.

IMPLEMENT_FACTORY(Date)
REGISTER_DATE(DateImpl)

IMPLEMENT_FACTORY(Person)
REGISTER_PERSON(PersonImpl)

IMPLEMENT_FACTORY(Animal)
REGISTER_ANIMAL(Cow)
REGISTER_ANIMAL(Sheep)

IMPLEMENT_FACTORY(PetOwner)
REGISTER_PET_OWNER(HumanPetOwner)

void Sheep::Init(StreamTokenizer &st, Environment *env) {
  int env_age;
  // Note how we need to cast Environment down to EnvironmentImpl,
  // because only the implementation has the templated \link
  // EnvironmentImpl::Get \endlink method, because only an
  // implementation can be aware of the all the \link
  // Factory\endlink-constructible types.
  EnvironmentImpl *env_impl = dynamic_cast<EnvironmentImpl *>(env);
  if (env_impl != nullptr &&
      env_impl->Get("age", &env_age)) {
    age_ = env_age * 2;
  }
}

}  // namespace infact
