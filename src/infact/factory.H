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

#ifndef INFACT_FACTORY_H_
#define INFACT_FACTORY_H_

#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>

#include "stream-tokenizer.H"
#include "environment.H"

namespace infact {

using std::cerr;
using std::endl;
using std::ostream;
using std::ostringstream;
using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

/// We use the templated class TypeName to be able to take an actual
/// C++ type and get the type name string used by the \link
/// infact::Interpreter Interpreter \endlink and \link
/// infact::Environment Environment \endlink classes.  This is so
/// that the \link infact::TypedMemberInitializer::Init Init
/// \endlink method of the \link infact::TypedMemberInitializer
/// TypedMemberInitializer \endlink class, defined below, can give the
/// \link infact::Environment::ReadAndSet Environment::ReadAndSet
/// \endlink method an explicit type name of the data member is about
/// to read and set.
///
/// The basic implementation here works for \link infact::Factory
/// Factory\endlink-constructible types, returning the result of the
/// \link infact::Factory::BaseName Factory::BaseName \endlink
/// method for an instance of <tt>Factory\<T\></tt>.
template <typename T>
class TypeName {
 public:
  string ToString() {
    return Factory<T>().BaseName();
  }
};

/// A partial specialization so that an object of type <tt>bool</tt>
/// converts to <tt>"bool"</tt>.
template <>
class TypeName<bool> {
 public:
  string ToString() {
    return "bool";
  }
};

/// A partial specialization so that an object of type <tt>int</tt>
/// converts to <tt>"int"</tt>.
template <>
class TypeName<int> {
 public:
  string ToString() {
    return "int";
  }
};

/// A partial specialization so that an object of type <tt>double</tt>
/// converts to <tt>"double"</tt>.
template <>
class TypeName<double> {
 public:
  string ToString() {
    return "double";
  }
};

/// A partial specialization so that an object of type <tt>string</tt>
/// converts to <tt>"string"</tt>.
template <>
class TypeName<string> {
 public:
  string ToString() {
    return "string";
  }
};

/// A partial specialization so that an object of type
/// <tt>shared_ptr\<T\></tt>, where <tt>T</tt> is some \link
/// infact::Factory Factory\endlink-constructible type, converts to
/// the string produced by <tt>TypeName\<T\></tt>.
template <typename T>
class TypeName<shared_ptr<T> > {
 public:
  string ToString() {
    return TypeName<T>().ToString();
  }
};

/// A partial specialization so that an object of type
/// <tt>vector\<T\></tt> gets converted to the type name of <tt>T</tt>
/// followed by the string <tt>"[]"</tt>, equivalent to the result of
/// executing the following expression:
/// \code
/// TypeName<T>().ToString() + "[]";
/// \endcode
template <typename T>
class TypeName<vector<T> > {
 public:
  string ToString() {
    return TypeName<T>().ToString() + "[]";
  }
};

/// \class MemberInitializer
///
/// An interface for data member initializers of members of a \link
/// infact::Factory Factory\endlink-constructible object.
class MemberInitializer {
 public:
  /// Initializes this base class.
  ///
  /// \param name     the name of the member to be initialized, as it should
  //                  appear in spec strings parsed by the
  ///                 \link Factory::CreateOrDie \endlink method
  /// \param required whether this member is required to be initialized in a
  ///                 spec string
  MemberInitializer(const string &name, bool required) :
      name_(name), initialized_(0), required_(required) { }

  /// Destroys this instance.
  virtual ~MemberInitializer() { }

  /// Returns the name of the member initialized by this instance, as
  /// it should appear in a spec string parsed by \link
  /// Factory::CreateOrDie\endlink.  While it is not a requirement
  /// that this name be identical to the declared name of the member
  /// in its C++ class, the convention is to use the declared name
  /// without an underscore.
  virtual string Name() { return name_; }

  /// Initializes this instance based on the following tokens obtained from
  /// the specified \link StreamTokenizer\endlink.
  ///
  /// \param st  the stream tokenizer whose next tokens contain the information
  ///            to initialize this data member
  /// \param env the current environment, to be modified by this member&rsquo;s
  ///            initialization
  virtual void Init(StreamTokenizer &st, Environment *env) = 0;

  /// Returns the number of times this member initializer&rsquo;s
  /// \link Init \endlink method has been invoked.
  virtual int Initialized() const { return initialized_; }

  /// Whether this member is required to be initialized in a spec string.
  virtual bool Required() const { return required_; }

 protected:
  /// The name of this member.
  string name_;
  /// The number of times this member initializer&rsquo;s Init method has
  /// been invoked.
  int initialized_;
  /// Whether this member is required to be initialized.
  bool required_;
};

/// A concrete, typed implementation of the MemberInitializer base class.
/// This class holds a pointer to a particular type of data member from
/// some \link Factory\endlink-constructible class that it can initialize
/// from a \link StreamTokenizer \endlink instance.
///
/// \tparam T a type of data member from a \link Factory\endlink-constructible
///           class that this class can initialize by reading tokens from
///           a \link StreamTokenizer \endlink instance
template <typename T>
class TypedMemberInitializer : public MemberInitializer {
 public:
  /// A concrete implementation of the MemberInitializer base class, holding
  /// a pointer to a typed member that needs to be initialized.
  ///
  /// \param name     the name of the member to be initialized, as it should
  //                  appear in spec strings parsed by the
  ///                 \link Factory::CreateOrDie \endlink method
  /// \param member   a pointer to a data member of a C++ class to be
  ///                 initialized, or nullptr if this initializer should only
  ///                 modify the Environment (providing a mapping between a
  ///                 name a value that might be used by a class&rsquo;
  ///         \link FactoryConstructible::Init(const Environment*,const string&)
  ///                 Init \endlink method
  /// \param required whether this member is required to be initialized in a
  ///                 spec string
  TypedMemberInitializer(const string &name, T *member, bool required = false) :
      MemberInitializer(name, required), member_(member) { }
  virtual ~TypedMemberInitializer() { }
  virtual void Init(StreamTokenizer &st, Environment *env) {
    env->ReadAndSet(Name(), st, TypeName<T>().ToString());
    if (member_ != nullptr) {
      VarMapBase *var_map = env->GetVarMap(Name());
      VarMap<T> *typed_var_map = dynamic_cast<VarMap<T> *>(var_map);
      if (typed_var_map != nullptr) {
	bool success = typed_var_map->Get(Name(), member_);
	if (success) {
	  ++initialized_;
	}
      }
    } else {
      // When the goal is simply to modify the environment, we say that this
      // "non-member" has been successfully initialized when we've modified
      // the environment.
      ++initialized_;
    }
  }
 protected:
  T *member_;
};

/// \class Initializers
///
/// A container for all the member initializers for a particular
/// Factory-constructible instance.  This class provides an easy, consistent
/// syntax for Factory-constructible classes to specify which members they
/// want/need initialized by the Factory based on the specification string.
class Initializers {
 public:
  /// Forward the <tt>const_iterator</tt> typedef of the internal data
  /// structure, to make code compact and readable.
  typedef unordered_map<string, MemberInitializer *>::const_iterator
      const_iterator;
  /// Forward the <tt>iterator</tt> typedef of the internal data
  /// structure, to make code compact and readable.
  typedef unordered_map<string, MemberInitializer *>::iterator iterator;

  /// Constructs a new instance.
  Initializers() { }
  /// Destroys this instance.
  virtual ~Initializers() {
    for (iterator init_it = initializers_.begin();
         init_it != initializers_.end();
         ++init_it) {
      delete init_it->second;
    }
  }

  /// This method is the raison d'etre of this class: a method to make it
  /// easy to add all supported types of members of a class to be initialized
  /// upon construction by its \link infact::Factory Factory\endlink.
  ///
  /// \param name     the name of the member to be initialized (as it
  ///                 should appear in the initialization spec string;
  ///                 note that this string does not have to be
  ///                 identical to the actual member name in the C++
  ///                 class definition, but the convention is to make
  ///                 it identical but without an underscore
  ///                 character)
  /// \param member   a pointer to the class instance's member that is
  ///                 to be initialized based on tokens in a spec
  ///                 string parsed by the \link
  ///                 infact::Factory::CreateOrDie
  ///                 Factory::CreateOrDie \endlink method
  /// \param required whether this member must be specified in a spec string
  template<typename T>
  void Add(const string &name, T *member, bool required = false) {
    if (initializers_.find(name) != initializers_.end()) {
      ostringstream err_ss;
      err_ss << "Initializers::Add: error: two members have the same name: "
	     << name;
      throw std::runtime_error(err_ss.str());
    }
    initializers_[name] = new TypedMemberInitializer<T>(name, member, required);
  }

  /// Returns a const iterator pointing to the beginning of the map
  /// from member names to pointers to \link infact::TypedMemberInitializer
  /// TypedMemberInitializer \endlink instances.
  const_iterator begin() const { return initializers_.begin(); }
  /// Returns a const iterator pointing to the end of the map from
  /// member names to pointers to \link
  /// infact::TypedMemberInitializer TypedMemberInitializer \endlink
  /// instances.
  const_iterator end() const { return initializers_.end(); }

  /// Returns an iterator pointing to the beginning of the map from
  /// member names to pointers to \link
  /// infact::TypedMemberInitializer TypedMemberInitializer \endlink
  /// instances.
  iterator begin() { return initializers_.begin(); }
  /// Returns an iterator pointing to the end of the map from member
  /// names to pointers to \link infact::TypedMemberInitializer
  /// TypedMemberInitializer \endlink instances.
  iterator end() { return initializers_.end(); }

  /// Returns a <tt>const_iterator</tt> pointing to the \link
  /// MemberInitializer \endlink associated with the specified name,
  /// or else \link end \endlink if no such \link
  /// infact::TypedMemberInitializer TypedMemberInitializer \endlink
  /// exists.
  const_iterator find(const string &name) const {
    return initializers_.find(name);
  }
  /// Returns an <tt>iterator</tt> pointing to the \link
  /// MemberInitializer \endlink associated with the specified name,
  /// or else \link end \endlink if no such \link
  /// infact::TypedMemberInitializer TypedMemberInitializer \endlink
  /// exists.
  iterator find(const string &name) {
    return initializers_.find(name);
  }
 private:
  unordered_map<string, MemberInitializer *> initializers_;
};

/// An interface for all \link Factory \endlink instances, specifying a few
/// pure virtual methods.
class FactoryBase {
 public:
  virtual ~FactoryBase() { }
  /// Clears the (possibly static) data of this factory.
  /// \p
  /// Note that invoking this method will prevent the factory from functioning!
  /// It should only be invoked when the factory is no longer needed by
  /// the current process.
  virtual void Clear() = 0;
  /// Returns the name of the base type of objects constructed by this factory.
  virtual const string BaseName() const = 0;
  /// Collects the names of types registered with this factory.
  ///
  /// \param[out] registered registered a set to be modified by this method
  ///                                   so that it contains the names of
  ///                                   concrete types registered with this
  ///                                   factory
  virtual void CollectRegistered(unordered_set<string> &registered) const = 0;

  virtual VarMapBase *CreateVarMap(Environment *env) const = 0;

  virtual VarMapBase *CreateVectorVarMap(Environment *env) const = 0;
};

/// A class to hold all \link Factory \endlink instances that have been created.
class FactoryContainer {
 public:
  typedef vector<FactoryBase *>::iterator iterator;

  /// Adds the specified factory to this container of factories.
  ///
  /// \param factory the factory to add to this container
  static void Add(FactoryBase *factory) {
    if (!initialized_) {
      factories_ = new vector<FactoryBase *>();
      initialized_ = 1;
    }
    factories_->push_back(factory);
  }
  /// Clears this container of factories.
  static void Clear() {
    if (initialized_) {
      for (vector<FactoryBase *>::iterator it = factories_->begin();
           it != factories_->end();
           ++it) {
        (*it)->Clear();
        delete *it;
      }
      delete factories_;
    }
  }

  // Provide two methods to iterate over the FactoryBase instances
  // held by this FactoryContainer.
  static iterator begin() {
    if (factories_ == nullptr) {
      cerr << "FactoryContainer::begin: error: no FactoryBase instances!"
           << endl;
    }
    return factories_->begin();
  }
  static iterator end() {
    if (factories_ == nullptr) {
      cerr << "FactoryContainer::begin: error: no FactoryBase instances!"
           << endl;
    }
    return factories_->end();
  }

  /// Prints the base typenames for all factories along with a list of all
  /// concrete subtypes those factories can construct, in a human-readable
  /// form, to the specified output stream.
  static void Print(ostream &os) {
    if (!initialized_) {
      return;
    }
    cerr << "Number of factories: " << factories_->size() << "." << endl;
    for (vector<FactoryBase *>::iterator factory_it = factories_->begin();
         factory_it != factories_->end();
         ++factory_it) {
      unordered_set<string> registered;
      (*factory_it)->CollectRegistered(registered);
      os << "Factory<" << (*factory_it)->BaseName() << "> can construct:\n";
      for (unordered_set<string>::const_iterator it = registered.begin();
           it != registered.end();
           ++it) {
        os << "\t" << *it << "\n";
      }
    }
    os.flush();
  }
 private:
  static int initialized_;
  static vector<FactoryBase *> *factories_;
};

/// \class Constructor
///
/// An interface with a single virtual method that constructs a
/// concrete instance of the abstract type <tt>T</tt>.
///
// \tparam T the abstract type that this <tt>Constructor</tt> constructs
template <typename T>
class Constructor {
 public:
  virtual ~Constructor() { }
  virtual T *NewInstance() const = 0;
};

/// An interface to make it easier to implement Factory-constructible
/// types by implementing both required methods to do nothing.
class FactoryConstructible {
 public:
  virtual ~FactoryConstructible() { }
  virtual void RegisterInitializers(Initializers &initializers) { }
  virtual void Init(const Environment *env, const string &arg) { }
};

/// Factory for dynamically created instance of the specified type.
///
/// \tparam T the type of objects created by this factory, required to
///           have the two methods defined in the \link
///           infact::FactoryConstructible FactoryConstructible
///           \endlink class
template <typename T>
class Factory : public FactoryBase {
 public:
  /// Constructs a new factory
  Factory() { }

  /// Clears this factory of all (possibly static) data.
  /// \p
  /// Note that invoking this method will prevent the factory from functioning!
  /// It should only be invoked when the factory is no longer needed by
  /// the current process.
  virtual void Clear() {
    ClearStatic();
  }

  /// Dynamically creates an object, whose type and initialization are
  /// contained in a specification string, the tokens of which are
  /// given by the specified \link StreamTokenizer\endlink.  A
  /// specification string has the form
  /// \code
  /// Typename(member1(init1), member2(init2), ...)
  /// \endcode
  /// where the type of a member can be
  /// <ul><li>a primitive (a <tt>string</tt>, <tt>double</tt>,
  ///         <tt>int</tt> or <tt>bool</tt>),
  ///     <li>a \link Factory\endlink-constructible type,
  ///     <li>a vector of primtives or
  ///     <li>a vector of types constructible by the same Factory.
  /// </ul>
  /// In the case of members that are vectors, the init string can be
  /// a comma-separated list of of initializers for its elements.  For
  /// example, the class Cow has two members
  /// that are registered to be initialized by \link
  /// Factory\endlink\<\link Cow\endlink\> (via the \link
  /// Cow::RegisterInitializers \endlink method):
  /// <ul><li>a member named <tt>name</tt> of type <tt>string</tt>,
  ///     <li>a member named <tt>age</tt> of type <tt>int</tt>
  /// </ul>
  /// Only the first of these is a &ldquo;required&rdquo; member, meaning
  /// the <tt>age</tt> member acts like an optional argument to a constructor.
  /// The following are all legal specification strings for constructing
  /// instances of Cow:
  /// \code
  /// Cow(name("foo"), age(3))
  /// Cow(age(3), name("foo"))
  /// Cow(name("foo"))
  /// \endcode
  /// Crucially, note how a vector can have an optional comma at the
  /// end of its list (the second example), and how a boolean may be
  /// initialized either with one of the two <i>reserved words</i>
  /// <tt>true</tt> or <tt>false</tt>, as in C and C++. Finally, unlike
  /// parameter lists to C++ constructors, since our members are always
  /// named, the grammar allows them to appear in any order, making the
  /// following two specification strings equivalent:
  /// \code
  /// ExampleFeatureExtractor(arg("foo"), strvec({"foo", "bar", "baz"}))
  /// ExampleFeatureExtractor(strvec({"foo", "bar", "baz"}), arg("foo"))
  /// \endcode
  ///
  /// More formally, the specification string must conform to the following
  /// grammar:
  /// <table border=0>
  /// <tr>
  ///   <td><tt>\<spec\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<type\> '(' \<member_init_list\> ')'</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<type\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td>name of type constructible by a Factory</td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<member_init_list\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<member_init\> [ ',' \<member_init\> ]* [',']</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<member_init\></tt>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<primitive_init\> | \<factory_init\> |
  ///           \<primitive_vector_init\> | \<factory_vector_init\> </tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<primitive_init\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<member_name\> '(' \<literal\> ')'</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<member_name\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td>the name of the member to be initialized, as specified by
  ///       <tt>\<type\>&rsquo;s</tt> <tt>RegisterInitializers</tt> method</td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<literal\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<string_literal\> | \<double_literal\> |
  ///           \<int_literal\> | \<bool_literal\></tt></td>
  /// </tr>
  /// <tr valign=top>
  ///   <td><tt>\<string_literal\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td>a C++ string literal (a string of characters surrounded by
  ///       double quotes); double quotes and backslashes may be
  ///       escaped inside a string literal with a backslash; other
  ///       escape sequences, such as <tt>\\t</tt> for the tab
  ///       character, are currently not recognized</td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<double_literal\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td>a string that can be parsed by <tt>atof</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<int_literal\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td>a string that can be parsed by <tt>atoi</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<bool_literal\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>true | false</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<primitive_vector_init></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<member_name\> '(' '{' \<literal_list\> '}' ')'</tt></td>
  /// </tr>
  /// <tr>
  ///   <td valign=top><tt>\<literal_list\></tt></td>
  ///   <td valign=top><tt>::=</tt></td>
  ///   <td><tt>\<string_literal\> [ ',' \<string_literal\> ]* [','] |<br>
  ///           \<double_literal\> [ ',' \<double_literal\> ]* [','] |<br>
  ///           \<int_literal\> [ ',' \<int_literal\> ]* [','] |<br>
  ///           \<bool_literal\> [ ',' \<bool_literal\> ]* [',']</tt>
  ///   </td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<factory_init\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<member_name\> '(' \<spec\> ')'</tt></td>
  /// </tr>
  /// <tr>
  ///   <td><tt>\<factory_vector_init\></tt></td>
  ///   <td><tt>::=</tt></td>
  ///   <td><tt>\<member_name\> '(' '{' \<spec_list\> '}' ')'</tt></td>
  /// </tr>
  /// <tr>
  ///   <td valign=top><tt>\<spec_list\></tt></td>
  ///   <td valign=top><tt>::=</tt></td>
  ///   <td><tt>\<spec\> [ ',' \<spec\> ]* [',']</tt><br>
  ///       where every <tt>\<spec\></tt> has a <tt>\<type\></tt>
  ///       constructible by the same Factory (<i>i.e.</i>, all
  ///       <tt>\<type\></tt>&rsquo;s have a common abstract base class)
  ///   </td>
  /// </tr>
  /// </table>
  ///
  /// \param st  the stream tokenizer providing tokens according to the
  ///            grammar shown above
  /// \param env the \link infact::Environment Environment \endlink in
  ///            this method was called, or <tt>nullptr</tt> if there is
  ///            no calling environment
  shared_ptr<T> CreateOrDie(StreamTokenizer &st, Environment *env = nullptr) {
    shared_ptr<Environment> env_ptr(env == nullptr ?
                                    Environment::CreateEmpty() : env->Copy());
    size_t start = st.PeekTokenStart();
    StreamTokenizer::TokenType token_type = st.PeekTokenType();
    if (token_type == StreamTokenizer::RESERVED_WORD &&
	(st.Peek() == "nullptr" || st.Peek() == "NULL")) {
      // Consume the nullptr.
      st.Next();
      return shared_ptr<T>();
    }
    if (token_type != StreamTokenizer::IDENTIFIER) {
      ostringstream err_ss;
      err_ss << "Factory<" << BaseName() << ">: "
             << "error: expected type specifier token but found "
             << StreamTokenizer::TypeName(token_type);
      throw std::runtime_error(err_ss.str());
    }

    // Read the concrete type of object to be created.
    string type = st.Next();

    // Read the open parenthesis token.
    if (st.Peek() != "(") {
      ostringstream err_ss;
      err_ss << "Factory<" << BaseName() << ">: "
             << "error: expected '(' at stream position "
             << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    st.Next();

    // Attempt to create an instance of type.
    typename unordered_map<string, const Constructor<T> *>::iterator cons_it =
        cons_table_->find(type);
    if (cons_it == cons_table_->end()) {
      ostringstream err_ss;
      err_ss << "Factory<" << BaseName() << ">: "
             << "error: unknown type: \"" << type << "\"";
      throw std::runtime_error(err_ss.str());
    }
    shared_ptr<T> instance(cons_it->second->NewInstance());

    // Ask new instance to set up member initializers.
    Initializers initializers;
    instance->RegisterInitializers(initializers);

    // Parse initializer list.
    while (st.Peek() != ")") {
      token_type = st.PeekTokenType();
      if (token_type != StreamTokenizer::IDENTIFIER) {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: expected token of type IDENTIFIER at "
               << "stream position " << st.PeekTokenStart() << " but found "
               << StreamTokenizer::TypeName(token_type) << ": \""
               << st.Peek() << "\"";
        throw std::runtime_error(err_ss.str());
      }
      size_t member_name_start = st.PeekTokenStart();
      string member_name = st.Next();
      typename Initializers::iterator init_it = initializers.find(member_name);
      if (init_it == initializers.end()) {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: unknown member name \"" << member_name
               << "\" in initializer list for type " << type << " at stream "
               << "position " << member_name_start;
        throw std::runtime_error(err_ss.str());
      }
      MemberInitializer *member_initializer = init_it->second;

      // Read open parenthesis.
      if (st.Peek() != "(") {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: expected '(' at stream position "
               << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
        throw std::runtime_error(err_ss.str());
      }
      st.Next();

      // Initialize member based on following token(s).
      member_initializer->Init(st, env_ptr.get());

      // Read close parenthesis for current member initializer.
      if (st.Peek() != ")") {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: expected ')' at stream position "
               << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
        throw std::runtime_error(err_ss.str());
      }
      st.Next();

      // Each member initializer must be followed by a comma or the final
      // closing parenthesis.
      if (st.Peek() != ","  && st.Peek() != ")") {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: expected ',' or ')' at stream position "
               << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
        throw std::runtime_error(err_ss.str());
      }
      // Read comma, if present.
      if (st.Peek() == ",") {
        st.Next();
      }
    }

    // Read the close parenthesis token for this factory type specification.
    if (st.Peek() != ")") {
      ostringstream err_ss;
      err_ss << "Factory<" << BaseName() << ">: "
             << "error: expected ')' at stream position "
             << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
      throw std::runtime_error(err_ss.str());
    }
    st.Next();

    // Run through all member initializers: if any are required but haven't
    // been invoked, it is an error.
    for (typename Initializers::const_iterator init_it = initializers.begin();
         init_it != initializers.end();
         ++init_it) {
      MemberInitializer *member_initializer = init_it->second;
      if (member_initializer->Required() &&
          member_initializer->Initialized() == 0) {
        ostringstream err_ss;
        err_ss << "Factory<" << BaseName() << ">: "
               << "error: initialization for member with name \""
               << init_it->first << "\" required but not found (current "
               << "stream position: " << st.tellg() << ")";
        throw std::runtime_error(err_ss.str());
      }
    }

    size_t end = st.tellg();
    // Invoke new instance's Init method.
    string stream_str = st.str();
    //cerr << "Full stream string is: \"" << stream_str << "\"" << endl;
    string init_str = stream_str.substr(start, end - start);
    //cerr << "Init string is: \"" << init_str << "\"" << endl;
    instance->Init(env_ptr.get(), init_str);

    return instance;
  }

  shared_ptr<T> CreateOrDie(const string &spec, const string err_msg,
                            Environment *env = nullptr) {
    StreamTokenizer st(spec);
    return CreateOrDie(st, env);
  }


  /// Returns the name of the base type of objects constructed by this factory.
  virtual const string BaseName() const { return base_name_; }

  /// Returns whether the specified type has been registered with this
  /// factory (where registration happens typically via the \link
  /// REGISTER_NAMED \endlink macro).
  ///
  /// \param type the type to be tested
  /// \return whether the specified type has been registered with this
  ///         factory
  static bool IsRegistered(const string &type) {
    return initialized_ && cons_table_->find(type) != cons_table_->end();
  }

  /// \copydoc FactoryBase::CollectRegistered
  virtual void CollectRegistered(unordered_set<string> &registered) const {
    if (initialized_) {
      for (typename unordered_map<string, const Constructor<T> *>::iterator it =
               cons_table_->begin();
           it != cons_table_->end();
           ++it) {
        registered.insert(it->first);
      }
    }
  }

  virtual VarMapBase *CreateVarMap(Environment *env) const {
    bool is_primitive = false;
    return new VarMap<shared_ptr<T> >(BaseName(), env, is_primitive);
  }

  virtual VarMapBase *CreateVectorVarMap(Environment *env) const {
    string name = BaseName() + "[]";
    bool is_primitive = false;
    return new VarMap<vector<shared_ptr<T> > >(name, BaseName(), env,
					       is_primitive);
  }

  /// The method used by the \link REGISTER_NAMED \endlink macro to ensure
  /// that subclasses add themselves to the factory.
  ///
  /// \param type the type to be registered
  /// \param p    the constructor for the specified type
  static const Constructor<T> *Register(const string &type,
                                        const Constructor<T> *p) {
    if (!initialized_) {
      cons_table_ = new unordered_map<string, const Constructor<T> *>();
      initialized_ = 1;
      FactoryContainer::Add(new Factory<T>());
    }
    (*cons_table_)[type] = p;
    return p;
  }

  /// Clears all static data associated with this class.
  /// \p
  /// Note that invoking this method will prevent the factory from functioning!
  /// It should only be invoked when the factory is no longer needed by
  /// the current process.
  static void ClearStatic() {
    if (initialized_) {
      for (typename unordered_map<string, const Constructor<T> *>::iterator it =
               cons_table_->begin();
           it != cons_table_->end();
           ++it) {
        delete it->second;
      }
      delete cons_table_;
      initialized_ = 0;
    }
  }
 private:
  // data members
  /// Initialization flag.
  static int initialized_;
  /// Factory map of prototype objects.
  static unordered_map<string, const Constructor<T> *> *cons_table_;
  static const char *base_name_;
};

/// A macro to define a subclass of \link infact::Constructor
/// Constructor \endlink whose NewInstance method constructs an
/// instance of \a TYPE, a concrete subclass of \a BASE.  The concrete
/// subclass \a TYPE must have a no-argument constructor.
/// \p
/// This is a helper macro used only by the <tt>REGISTER</tt> macro.
#define DEFINE_CONS_CLASS(TYPE,NAME,BASE) \
  class NAME ## Constructor : public Constructor<BASE> { \
   public: virtual BASE *NewInstance() const { return new TYPE(); } };

/// This macro registers the concrete subtype \a TYPE with the
/// specified factory for instances of type \a BASE; the \a TYPE is
/// associated with the specified \a NAME. This macro&mdash;or a macro
/// defined using this macro&mdash;should be used in the
/// implementation file for a concrete subclass \a TYPE of the
/// baseclass \a BASE.  Often, \a TYPE and \a NAME may be the exact
/// same string; however, they must be different when \a TYPE contains
/// characters that may not appear in C++ identifiers, such as colons
/// (<i>e.g.</i>, when \a TYPE is the fully-qualified name of an inner
/// class).
#define REGISTER_NAMED(TYPE,NAME,BASE)  \
  DEFINE_CONS_CLASS(TYPE,NAME,BASE) \
  const Constructor<BASE> *NAME ## _my_protoype = \
      Factory<BASE>::Register(string(#NAME), new NAME ## Constructor());

/// Provides the necessary implementation for a factory for the specified
/// <tt>BASE</tt> class type.
#define IMPLEMENT_FACTORY(BASE) \
  template<> int Factory<BASE>::initialized_ = 0; \
  template<> unordered_map<string, const Constructor<BASE> *> * \
    Factory<BASE>::cons_table_ = 0; \
  template<> const char *Factory<BASE>::base_name_ = #BASE;

}  // namespace infact

#endif
