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
/// Provides an environment for variables and their values, either primitive
/// or Factory-constructible objects, or vectors thereof.
/// \author dbikel@google.com (Dan Bikel)

#ifndef INFACT_ENVIRONMENT_IMPL_H_
#define INFACT_ENVIRONMENT_IMPL_H_

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>

#include "environment.H"

namespace infact {

using std::ostringstream;
using std::string;
using std::unordered_map;
using std::unordered_set;

/// Provides a set of named variables and their types, as well as the values
/// for those variables.
///
/// \see Interpreter
class EnvironmentImpl : public Environment {
 public:
  /// Constructs a new, empty environment.
  ///
  /// \param debug the debug level; if greater than 0, various debugging
  ///              messages will be output to <tt>std::cerr</tt>
  EnvironmentImpl(int debug = 0);

  /// Destroys this environment.
  virtual ~EnvironmentImpl() {
    for (unordered_map<string, VarMapBase *>::iterator it = var_map_.begin();
         it != var_map_.end(); ++it) {
      delete it->second;
    }
  }

  /// Returns whether the specified variable has been defined in this
  /// environment.
  virtual bool Defined(const string &varname) const {
    unordered_map<string, string>::const_iterator it = types_.find(varname);
    return it != types_.end();
  }

  /// Sets the specified variable to the value obtained from the following
  /// tokens available from the specified token stream.
  virtual void ReadAndSet(const string &varname, StreamTokenizer &st,
                          const string type);

  virtual const string &GetType(const string &varname) const {
    unordered_map<string, string>::const_iterator type_it =
        types_.find(varname);
    if (type_it == types_.end()) {
      // Error or warning.
    }
    return type_it->second;
  }

  virtual VarMapBase *GetVarMap(const string &varname) {
    return GetVarMapForType(GetType(varname));
  }

  /// Retrieves the VarMap instance for the specified type.
  virtual VarMapBase *GetVarMapForType(const string &type) {
    string lookup_type = type;
    // First, check if this is a concrete Factory-constructible type.
    // If so, map to its abstract type name.
    unordered_map<string, string>::const_iterator factory_type_it =
        concrete_to_factory_type_.find(type);
    if (factory_type_it != concrete_to_factory_type_.end()) {
      lookup_type = factory_type_it->second;
    }

    unordered_map<string, VarMapBase *>::const_iterator var_map_it =
        var_map_.find(lookup_type);
    if (var_map_it == var_map_.end()) {
      return nullptr;
    }
    return var_map_it->second;
  }

  /// \copydoc infact::Environment::Print
  virtual void Print(ostream &os) const {
    for (unordered_map<string, VarMapBase *>::const_iterator var_map_it =
             var_map_.begin();
         var_map_it != var_map_.end(); ++var_map_it) {
      var_map_it->second->Print(os);
    }
  }

  /// \copydoc infact::Environment::PrintFactories
  virtual void PrintFactories(ostream &os) const;

  /// \copydoc infact::Environment::Copy
  virtual Environment *Copy() const {
    EnvironmentImpl *new_env = new EnvironmentImpl(*this);
    // Now go through and create copies of each VarMap.
    for (unordered_map<string, VarMapBase *>::iterator new_env_var_map_it =
             new_env->var_map_.begin();
         new_env_var_map_it != new_env->var_map_.end(); ++new_env_var_map_it) {
      new_env_var_map_it->second = new_env_var_map_it->second->Copy(new_env);
    }
    return new_env;
  }

  /// Retrieves the value of the variable with the specified name and puts
  /// into into the object pointed to by the <tt>value</tt> parameter.
  ///
  /// \param      varname the name of the variable whose value is to be
  ///             retrieved
  /// \param[out] value a pointer to the object whose value is to be set
  /// \return whether the specified variable exists and its value was
  ///         successfully set by this method
  template<typename T>
  bool Get(const string &varname, T *value) const;

 private:
  /// Infer the type based on the next token and its token type.
  string InferType(const string &varname,
                   const StreamTokenizer &st, bool is_vector,
                   bool *is_object_type);

  /// A map from all variable names to their types.
  unordered_map<string, string> types_;

  /// A map from type name strings (as returned by the \link TypeName \endlink
  /// method) to VarMap instances for those types.
  unordered_map<string, VarMapBase *> var_map_;

  /// A map from concrete Factory-constructible type names to their abstract
  /// Factory type names.
  unordered_map<string, string> concrete_to_factory_type_;

  int debug_;
};

template<typename T>
bool
EnvironmentImpl::Get(const string &varname, T *value) const {
  unordered_map<string, string>::const_iterator type_it =
      types_.find(varname);
  if (type_it == types_.end()) {
    if (debug_ >= 1) {
      ostringstream err_ss;
      err_ss << "Environment::Get: error: no value for variable "
             << varname;
      cerr << err_ss.str() << endl;
    }
    return false;
  }

  // Now that we have the type, look up the VarMap.
  const string &type = type_it->second;
  unordered_map<string, VarMapBase*>::const_iterator var_map_it =
      var_map_.find(type);

  if (var_map_it == var_map_.end()) {
    ostringstream err_ss;
    err_ss << "Environment::Get: error: types_ and var_map_ data members "
           << "are out of sync";
    throw std::runtime_error(err_ss.str());
  }

  // Do a dynamic_cast down to the type-specific VarMap.
  VarMapBase *var_map = var_map_it->second;
  VarMap<T> *typed_var_map = dynamic_cast<VarMap<T> *>(var_map);

  if (typed_var_map == nullptr) {
    ostringstream err_ss;
    err_ss << "Environment::Get: error: no value for variable "
           << varname << " of type " << typeid(*value).name()
           << "; perhaps you meant " << type << "?";
    cerr << err_ss.str() << endl;
    return false;
  }
  bool success = typed_var_map->Get(varname, value);
  if (!success) {
    ostringstream err_ss;
    err_ss << "Environment::Get: error: no value for variable "
           << varname << " of type " << typeid(*value).name()
           << "; types_ and var_map_ data members are out of sync";
    throw std::runtime_error(err_ss.str());
  }
  return success;
  }

}  // namespace infact

#endif
