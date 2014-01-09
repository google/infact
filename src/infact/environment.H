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
/// Provides an interface for an Environment, which contains a map
/// from variables of a specific type (primitives, \link
/// infact::Factory Factory\endlink-constructible objects, or
/// vectors thereof) to their values.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_ENVIRONMENT_H_
#define INFACT_ENVIRONMENT_H_

#define VAR_MAP_DEBUG 0

#include <sstream>
#include <vector>

#include "stream-init.H"
#include "stream-tokenizer.H"

namespace infact {

using std::cerr;
using std::endl;
using std::ostream;
using std::ostringstream;
using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class Environment;

/// A base class for a mapping from variables of a specific type to their
/// values.
class VarMapBase {
 public:
  /// Constructs a base class for a concrete implementation providing
  /// a mapping from variables of a particular type to their values.
  ///
  /// \param name         the type name of the variables in this instance
  /// \param env          the \link infact::Environment Environment \endlink
  ///                     that wraps this VarMapBase instance
  /// \param is_primitive whether this instance contains primitive or primitive
  ///                     vector variables
  VarMapBase(const string &name, Environment *env, bool is_primitive) :
      name_(name), env_(env), is_primitive_(is_primitive) { }

  virtual ~VarMapBase() { }

  /// Returns whether this instance contains primitive or primtive
  /// vector variables.
  virtual bool IsPrimitive() const { return is_primitive_; }

  /// Returns the type name of the variables of this instance.
  virtual const string &Name() const { return name_; }

  /// Returns whether the specified variable has a definition in this
  /// environment.
  virtual bool Defined(const string &varname) const = 0;

  /// Reads the next value (primitive or spec for constructing a \link
  /// infact::Factory Factory\endlink-constructible object) from the
  /// specified stream tokenizer and sets the specified variable to
  /// that value.
  virtual void ReadAndSet(const string &varname, StreamTokenizer &st) = 0;

  /// Prints out a human-readable string to the specified output
  /// stream containing the variables, their type and, if primitive,
  /// their values.
  virtual void Print(ostream &os) const = 0;

  /// Returns a newly constructed copy of this VarMap.
  virtual VarMapBase *Copy(Environment *env) const = 0;

 protected:
  /// To allow proper implementation of Copy in VarMapBase implementation,
  /// since we don't get copying of base class' members for free.
  void SetMembers(const string &name, Environment *env, bool is_primitive) {
    name_ = name;
    env_ = env;
    is_primitive_ = is_primitive;
  }

  /// The type name of this VarMap.
  string name_;
  /// The Environment that holds this VarMap instance.
  Environment *env_;
  /// Whether this VarMap instance holds variables of primitive type
  /// or vector of primitives.
  bool is_primitive_;
};

/// An interface for an environment in which variables of various
/// types are mapped to their values.
class Environment {
 public:
  virtual ~Environment() { }

  /// Returns whether the specified variable has been defined in this
  /// environment.
  virtual bool Defined(const string &varname) const = 0;

  /// Sets the specified variable to the value obtained from the following
  /// tokens available from the specified token stream.
  virtual void ReadAndSet(const string &varname, StreamTokenizer &st,
                          const string type = "") = 0;

  /// Retrieves the type name of the specified variable.
  virtual const string &GetType(const string &varname) const = 0;

  /// Retrieves the VarMap instance for the specified variable.
  virtual VarMapBase *GetVarMap(const string &varname) = 0;

  /// Retrieves the VarMap instance for the specified type, or nullptr if
  /// there is no such VarMap.  The specified type must either be a
  /// primitive type, a primitive vector type, a \link
  /// infact::Factory Factory\endlink-constructible type or a vector
  /// of \link infact::Factory Factory\endlink-constructible types.
  ///
  /// If the specified type is a concrete implementation of a \link
  /// infact::Factory Factory\endlink-constructible type, then the
  /// pointer to the VarMap for its abstract base type is returned;
  /// for example, if the specified type is <tt>"RankFeatureExtactor"</tt>
  /// then this method will return the VarMap containing
  /// <tt>"FeatureExtractor"</tt> instances.
  ///
  /// TODO(dbikel): Support returning the VarMap for vectors of
  /// abstract type when the user specifies a vector of concrete type.
  /// E.g., if the specified type is "RankFeatureExtractor[]" then
  /// this method should return the VarMap containing
  /// "FeatureExtractor[]" instances.
  virtual VarMapBase *GetVarMapForType(const string &type) = 0;

  /// Prints a human-readable string of all the variables in this environment,
  /// their types and, if primitive, their values.
  virtual void Print(ostream &os) const = 0;

  /// Returns a copy of this environment.
  virtual Environment *Copy() const = 0;

  /// Prints out a human-readable string with the names of all abstract base
  /// types and their concrete implementations that may be constructed.
  /// \see infact::FactoryContainer::Print
  virtual void PrintFactories(ostream &os) const = 0;

  /// A static factory method to create a new, empty Environment instance.
  static Environment *CreateEmpty();
};

/// A template class that helps print out values with ostream& operator
/// support and vectors of those values.
///
/// \tparam T the type to print to an ostream
template <typename T>
class ValueString {
 public:
  string ToString(const T &value) const {
    ostringstream oss;
    oss << value;
    return oss.str();
  }
};

/// A specialization of the ValueString class to support printing of
/// string values.
template<>
class ValueString<string> {
 public:
  string ToString(const string &value) const {
    return "\"" + value + "\"";
  }
};

/// A partial specialization of the ValueString class to support printing of
/// boolean values.
template<>
class ValueString<bool> {
 public:
  string ToString(const bool &value) const {
    return value ? "true" : "false";
  }
};

/// A partial specialization of the ValueString class to support
/// printing of shared_ptr's to objects, where we simply print the typeid
/// name followed by a colon character followed by the pointer address.
template<typename T>
class ValueString<shared_ptr<T> > {
 public:
  string ToString(const shared_ptr<T> &value) const {
    ostringstream oss;
    oss << "<" << typeid(shared_ptr<T>).name() << ":" << value.get() << ">";
    return oss.str();
  }
};

/// A partial specialization of the ValueString class to support
/// printing of vectors of values.
///
/// \tparam T the element type for a vector to be printed out to an ostream
template <typename T>
class ValueString<vector<T> > {
 public:
  string ToString(const vector<T> &value) const {
    ostringstream oss;
    oss << "{";
    typename vector<T>::const_iterator it = value.begin();
    ValueString<T> value_string;
    if (it != value.end()) {
      oss << value_string.ToString(*it);
      ++it;
    }
    for (; it != value.end(); ++it) {
      oss << ", " << value_string.ToString(*it);
    }
    oss << "}";
    return oss.str();
  }
};

/// A partial implementation of the VarMapBase interface that is common
/// to both VarMap<T> and the VarMap<vector<T> > partial specialization.
///
/// In a delightful twist, this partial implementation of VarMapBase
/// uses the curiously recurring template pattern (CRTP) in order to
/// implement the \link infact::VarMapBase::Copy VarMapBase::Copy
/// \endlink method, as well as the protected method \link
/// ReadAndSetFromExistingVariable\endlink.
///
/// \tparam T       the type of variables stored in this (partial
///                 implementation of a) variable map
/// \tparam Derived the type of the class using this as its base class
///                 (a.k.a. CRTP, or the "curiously recurring template pattern")
template <typename T, typename Derived>
class VarMapImpl : public VarMapBase {
 public:
  VarMapImpl(const string &name, Environment *env, bool is_primitive = true) :
      VarMapBase(name, env, is_primitive) { }

  virtual ~VarMapImpl() { }

  // Accessor methods

  /// Assigns the value of the specified variable to the object pointed to
  /// by the <tt>value</tt> parameter.
  ///
  /// \return whether the specified variable exists and the assignment
  ///         was successful
  bool Get(const string &varname, T *value) const {
    typename unordered_map<string, T>::const_iterator it = vars_.find(varname);
    if (it == vars_.end()) {
      return false;
    } else {
      *value = it->second;
      return true;
    }
  }

  /// \copydoc VarMapBase::Defined
  virtual bool Defined(const string &varname) const {
    return vars_.find(varname) != vars_.end();
  }

  /// Sets the specified variable to the specified value.
  void Set(const string &varname, T value) {
    vars_[varname] = value;
  }

  /// \copydoc VarMapBase::Print
  virtual void Print(ostream &os) const {
    ValueString<T> value_string;
    for (typename unordered_map<string, T>::const_iterator it = vars_.begin();
         it != vars_.end(); ++it) {
      const T& value = it->second;
      os << Name() << " "
         << it->first
         << " = "
         << value_string.ToString(value)
         << ";\n";
    }
    os.flush();
  }

  /// \copydoc VarMapBase::Copy
  virtual VarMapBase *Copy(Environment *env) const {
    // Invoke Derived class' copy constructor.
    const Derived *derived = dynamic_cast<const Derived *>(this);
    if (derived == nullptr) {
      throw std::runtime_error("bad dynamic cast");
    }
    Derived *var_map_copy = new Derived(*derived);
    var_map_copy->SetMembers(name_, env, is_primitive_);
    return var_map_copy;
  }
 protected:
  /// Checks if the next token is an identifier and is a variable in
  /// the environment, and, if so, sets varname to the variable&rsquo;s value.
  bool ReadAndSetFromExistingVariable(const string &varname,
                                      StreamTokenizer &st) {
    if (env()->Defined(st.Peek())) {
      VarMapBase *var_map = env()->GetVarMap(st.Peek());
      Derived *typed_var_map = dynamic_cast<Derived *>(var_map);
      if (typed_var_map != nullptr) {
        // Finally consume variable.
        string rhs_variable = st.Next();
        T value = T();
        // Retrieve rhs variable's value.
        bool success = typed_var_map->Get(rhs_variable, &value);
        // Set varname to the same value.
        if (VAR_MAP_DEBUG >= 1) {
          cerr << "VarMap<" << Name() << ">::ReadAndSet: "
               << "setting variable "
               << varname << " to same value as rhs variable " << rhs_variable
               << endl;
        }
        if (success) {
          Set(varname, value);
        } else {
          // Error: we couldn't find the varname in this VarMap.
          if (VAR_MAP_DEBUG >= 1) {
            cerr << "VarMap<" << Name() << ">::ReadAndSet: no variable "
                 << rhs_variable << " found " << endl;
          }
        }
      } else {
        // Error: inferred or declared type of varname is different
        // from the type of the rhs variable.
        if (VAR_MAP_DEBUG >= 1) {
          cerr << "VarMap<" << Name() << ">::ReadAndSet: variable "
               << st.Peek() << " is of type " << var_map->Name()
               << " but expecting " << typeid(T).name() << endl;
        }
      }
      return true;
    } else {
      return false;
    }
  }

  /// A protected method to access the environment contained by this
  /// VarMapBase instance, for the two concrete VarMap implementations, below.
  Environment *env() { return VarMapBase::env_; }

 private:
  unordered_map<string, T> vars_;
};

/// A container to hold the mapping between named variables of a specific
/// type and their values.
///
/// \tparam T the type of variables held by this variable map
template <typename T>
class VarMap : public VarMapImpl<T, VarMap<T> > {
 public:
  typedef VarMapImpl<T, VarMap<T> > Base;

  /// Constructs a mapping from variables of a particular type to their values.
  ///
  /// \param name         the type name of the variables in this instance
  /// \param env          the \link infact::Environment Environment \endlink
  ///                     that contains this VarMap instance
  /// \param is_primitive whether this instance contains primitive or primitive
  ///                     vector variables
  VarMap(const string &name, Environment *env, bool is_primitive = true) :
      Base(name, env, is_primitive) { }

  virtual ~VarMap() { }

  /// \copydoc VarMapBase::ReadAndSet
  virtual void ReadAndSet(const string &varname, StreamTokenizer &st) {
    if (VAR_MAP_DEBUG >= 1) {
      cerr << "VarMap<" << Base::Name() << ">::ReadAndSet: "
           << "about to set varname " << varname << " of type "
           << typeid(T).name()
           << "; prev_token=" << st.PeekPrev() << "; next_tok=" << st.Peek()
           << endl;
    }

    if (!Base::ReadAndSetFromExistingVariable(varname, st)) {
      T value;
      Initializer<T> initializer(&value);
      initializer.Init(st, Base::env());
      this->Set(varname, value);

      if (VAR_MAP_DEBUG >= 1) {
        ValueString<T> value_string;
        cerr << "VarMap<" << Base::Name() << ">::ReadAndSet: set varname "
             << varname << " to value " << value_string.ToString(value)<< endl;
      }
    }
  }
};

/// A partial specialization to allow initialization of a vector of
/// values, where the values can either be literals (if T is a
/// primitive type), spec strings for constructing
/// \link Factory\endlink-constructible objects, or variable names (where each
/// variable must be of type T).
///
/// TODO(dbikel): Avoid repeating much of the unchanged implementation
/// from the non-specialized VarMap<T>, above, by using the PIMPL
/// design pattern (i.e., create VarMapImpl<T> that does the generic
/// stuff and have VarMap<T> contain a VarMapImpl<T> data member, and
/// have VarMap<vector<T>> contain a VarMapImpl<vector<T>> data
/// member.
///
/// \tparam T the element type for vectors stored in this variable map
template <typename T>
class VarMap<vector<T> > : public VarMapImpl<vector<T>, VarMap<vector<T> > > {
 public:
  typedef VarMapImpl<vector<T>, VarMap<vector<T> > > Base;

  /// Constructs a mapping from variables of a particular type to their values.
  ///
  /// \param name             the type name of the variables in this instance
  /// \param element_typename the type name of the elements of the vectors
  ///                         held by this instance
  /// \param env              the \link infact::Environment Environment \endlink
  ///                         that contains this VarMap instance
  /// \param is_primitive     whether the variables held in this variable map
  ///                         are primitives or vectors of primitives
  VarMap(const string &name, const string &element_typename, Environment *env,
	 bool is_primitive = true)
      : Base(name, env, is_primitive), element_typename_(element_typename) { }

  virtual ~VarMap() { }

  virtual void ReadAndSet(const string &varname, StreamTokenizer &st) {
    // First check if next token is an identifier and is a variable in
    // the environment, set varname to its value.
    if (!Base::ReadAndSetFromExistingVariable(varname, st)) {
      // This entire block reads the array of values.

      // Either the next token is an open brace (if reading tokens from
      // within a Factory-constructible object's member init list), or
      // else we just read an open brace (if Interpreter is reading tokens).
      if (st.Peek() == "{") {
        // Consume open brace.
        st.Next();
      } else {
        ostringstream err_ss;
        err_ss << "VarMap<vector<T>>: "
               << "error: expected '{' at stream position "
               << st.PeekPrevTokenStart() << " but found \""
               << st.PeekPrev() << "\"";
        throw std::runtime_error(err_ss.str());
      }

      vector<T> value;
      int element_idx = 0;
      while (st.Peek() != "}") {
        // Copy the environment, since we create fake names for each element.
        shared_ptr<Environment> env_ptr(Base::env()->Copy());
        ostringstream element_name_oss;
        element_name_oss << "____" << varname << "_" << (element_idx++)
                         << "____";
        string element_name = element_name_oss.str();

        env_ptr->ReadAndSet(element_name, st, element_typename_);
        VarMapBase *element_var_map =
            env_ptr->GetVarMapForType(element_typename_);
        VarMap<T> *typed_element_var_map =
            dynamic_cast<VarMap<T> *>(element_var_map);
        T element;
        bool success = typed_element_var_map->Get(element_name, &element);
        if (!success) {
          ostringstream err_ss;
          err_ss << "VarMap<" << Base::Name() << ">::ReadAndSet: trouble "
                 << "initializing element " << (element_idx - 1)
                 << " of variable " << varname;
          throw std::runtime_error(err_ss.str());
        }
        value.push_back(element);
        // Each vector element initializer must be followed by a comma
        // or the final closing parenthesis.
        if (st.Peek() != ","  && st.Peek() != "}") {
          ostringstream err_ss;
          err_ss << "Initializer<vector<T>>: "
                 << "error: expected ',' or '}' at stream position "
                 << st.PeekTokenStart() << " but found \"" << st.Peek() << "\"";
          throw std::runtime_error(err_ss.str());
        }
        // Read comma, if present.
        if (st.Peek() == ",") {
          st.Next();
        }
      }
      // Consume close brace.
      st.Next();


      // Finally, set the newly-constructed value.
      this->Set(varname, value);
    }
  }
 private:
  string element_typename_;
};

}  // namespace infact

#endif
