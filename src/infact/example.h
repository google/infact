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
/// Provides various example class headers.  Please note the presence
/// of the various <tt>REGISTER_*</tt> macros sprinkled throughout
/// this file.  The convention is for a <tt>REGISTER</tt>_<i>type</i>
/// macro to be placed just below the declaration of the <i>type</i>
/// interface declaration.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_EXAMPLE_H_
#define INFACT_EXAMPLE_H_

#include <string>

#include "factory.h"

namespace infact {

/// An interface to represent a date.
class Date : public FactoryConstructible {
 public:
  /// Constructs this date.
  Date() {}
  /// Destroys this date.
  virtual ~Date() {}

  /// Returns the year.
  virtual int year() const = 0;
  /// Returns the month.
  virtual int month() const = 0;
  /// Returns the day.
  virtual int day() const = 0;
};

/// The macro by which \link infact::Date Date \endlink implementations may
/// register themselves with the \link infact::Factory Factory \endlink that can
/// construct \link infact::Date Date \endlink instances.
#define REGISTER_DATE(TYPE) \
  REGISTER_NAMED(TYPE,TYPE,Date)

/// An implementation of the Date interface that can be constructed by
/// a \link Factory \endlink (because of the
/// <tt>REGISTER_DATE(DateImpl)</tt> declaration in
/// <tt>example.cc</tt>).
class DateImpl : public Date {
 public:
  /// Constructs this instance.
  DateImpl() : Date() {}
  /// Destroys this instance.
  virtual ~DateImpl() {}

  /// Registers three required initializers.
  virtual void RegisterInitializers(Initializers &initializers) {
    INFACT_ADD_REQUIRED_PARAM_(year);
    INFACT_ADD_REQUIRED_PARAM_(month);
    INFACT_ADD_REQUIRED_PARAM_(day);
  }

  /// \copydoc Date::year
  virtual int year() const { return year_; }
  /// \copydoc Date::month
  virtual int month() const { return month_; }
  /// \copydoc Date::day
  virtual int day() const { return day_; }

 private:
  int year_;
  int month_;
  int day_;
};

/// An interface representing a person.
class Person : public FactoryConstructible {
 public:
  /// Constructs this abstract person.
  Person() {}
  /// Destroys this abstract person.
  virtual ~Person() {}

  /// Returns the name of this person.
  virtual const string &name() const = 0;
  /// Returns the height in centimeters of this person.
  virtual const int cm_height() const = 0;
  /// Returns the birthday of this person.
  virtual shared_ptr<const Date> birthday() const = 0;
};

/// The macro by which \link infact::Person Person \endlink
/// implementations may register themselves with the \link
/// infact::Factory Factory \endlink that can construct \link
/// infact::Person Person \endlink instances.
#define REGISTER_PERSON(TYPE) \
  REGISTER_NAMED(TYPE,TYPE,Person)

/// A concrete implementation of the Person interface that can be
/// constructed by a \link Factory \endlink (because of the
/// <tt>REGISTER_PERSON(PersonImpl)</tt> declaration in
/// <tt>example.cc</tt>).
class PersonImpl : public Person {
 public:
  /// Constructs this person.
  PersonImpl() : Person() {}
  /// Destroys this person.
  virtual ~PersonImpl() {}

  /// Registers one required and two optional initializers.
  virtual void RegisterInitializers(Initializers &initializers) {
    INFACT_ADD_REQUIRED_PARAM_(name);
    INFACT_ADD_PARAM_(cm_height);
    INFACT_ADD_PARAM_(birthday);
  }

  /// \copydoc Person::name
  virtual const string &name() const { return name_; }
  /// \copydoc Person::cm_height
  virtual const int cm_height() const { return cm_height_; }
  /// \copydoc Person::birthday
  virtual shared_ptr<const Date> birthday() const { return birthday_; }

 private:
  string name_;
  int cm_height_;
  shared_ptr<Date> birthday_;
};

/// A very simple class to represent an animal.
class Animal : public FactoryConstructible {
 public:
  /// Constructs a generic animal.
  Animal() {}
  /// Destroys this animal.
  virtual ~Animal() {}

  /// Returns the name of this animal.
  virtual const string &name() const = 0;

  /// Returns the age of this animal.
  virtual int age() const = 0;
};

/// Registers the \link infact::Animal Animal \endlink with the
/// specified subtype <tt>TYPE</tt> and <tt>NAME</tt> with the \link
/// infact::Animal Animal \endlink \link infact::Factory
/// Factory\endlink.
#define REGISTER_ANIMAL(TYPE) \
  REGISTER_NAMED(TYPE,TYPE,Animal)

/// A class to represent a cow.
class Cow : public Animal {
public:
  /// Constructs a cow.
  Cow() : Animal() {
    age_ = 2; // default age, since age is optional
  }

  // Destroys this instance.
  virtual ~Cow() {}

  virtual void RegisterInitializers(Initializers &initializers) {
    INFACT_ADD_REQUIRED_PARAM_(name);
    INFACT_ADD_PARAM_(age);
  }

  /// Returns the name of this animal.
  virtual const string &name() const { return name_; }
  virtual int age() const { return age_; }
private:
  string name_;
  int age_;
};

/// A sheep.  Unlike other animals, sheep are always twice the age you
/// specify (for the purposes of this example, anyway).  Please see
/// how we do this by looking at the implementations of the \link
/// infact::Sheep::RegisterInitializers RegisterInitializers \endlink
/// and \link infact::Sheep::PostInit PostInit \endlink methods.
class Sheep : public Animal {
 public:
  /// Constructs a sheep.
  Sheep() : Animal() {}
  /// Destroys a sheep.
  virtual ~Sheep() {}

  /// Registers one required and two optional inititalizers.  Please
  /// note the use of a temporary variable being initialized in this
  /// method, for use inside the \link infact::Sheep::PostInit
  /// PostInit \endlink method.
  virtual void RegisterInitializers(Initializers &initializers) {
    INFACT_ADD_REQUIRED_PARAM_(name);
    INFACT_ADD_PARAM_(counts);
    // We can pass in a nullptr, if we don't want to directly
    // initialize a data member of this class, but instead want to
    // simply grab something from the Environment after this method
    // has been invoked; see the PostInit method declared below
    // (defined in example.cc).
    //
    // As of v1.0.6, the new INFACT_ADD_TEMPORARY macro makes it very
    // easy to add such temporary variables in a very readable way.
    INFACT_ADD_TEMPORARY(int, age);
  }

  /// Grabs the variable named "age" from the Environment (set up by
  /// \link infact::Sheep::RegisterInitializers
  /// RegisterInitializers\endlink) and sets this sheep&rsquo;s age to
  /// be twice that value.
  virtual void PostInit(const Environment *env, const string &init_str);

  /// \copydoc Animal::name
  virtual const string &name() const { return name_; }
  /// \copydoc Animal::age
  virtual int age() const {
    return -1;  // Always returns -1, because sheep are timeless.
  }
  /// Returns the counts of this sheep.
  const vector<int> &counts() { return counts_; }
 private:
  string name_;
  int age_;
  // The various times people have counted this sheep when falling asleep.
  vector<int> counts_;
};

/// An owner of a pet.
class PetOwner : public FactoryConstructible {
 public:
  /// Constructs a generic pet owner.
  PetOwner() {}

  /// Destroys this pet owner.
  virtual ~PetOwner() {}

  /// Returns the number of pets owned by this pet owner.
  virtual int GetNumberOfPets() = 0;

  /// Gets the pet with the specified index owned by this pet owner.
  virtual shared_ptr<Animal> GetPet(int i) = 0;
};

/// The macro by which \link infact::PetOwner PetOwner \endlink
/// implementations may register themselves with the \link
/// infact::Factory Factory \endlink that can construct \link
/// infact::PetOwner PetOwner \endlink instances.
#define REGISTER_PET_OWNER(TYPE) \
  REGISTER_NAMED(TYPE,TYPE,PetOwner)

/// A concrete type of PetOwner that can be constructed by a \link
/// Factory\endlink\<\link PetOwner \endlink\> instance.
class HumanPetOwner : public PetOwner {
 public:
  /// Constructs a human pet owner.
  HumanPetOwner() : PetOwner() {}

  /// Destroys a human pet owner.
  virtual ~HumanPetOwner() {}

  /// Registers a single required initializer.
  virtual void RegisterInitializers(Initializers &initializers) {
    initializers.Add("pets", &pets_, true);
  }

  /// Returns the number of pets owned by this pet owner.
  virtual int GetNumberOfPets() {
    return pets_.size();
  }

  /// Gets the pet with the specified index owned by this pet owner.
  virtual shared_ptr<Animal> GetPet(int i) {
    return pets_.at(i);
  }

 private:
  vector<shared_ptr<Animal> > pets_;
};

}  // namespace infact

#endif
