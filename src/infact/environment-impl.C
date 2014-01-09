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
/// Implementation of the Environment class.
/// \author dbikel@google.com (Dan Bikel)

#include "environment-impl.H"
#include "factory.H"

namespace infact {

EnvironmentImpl::EnvironmentImpl(int debug) {
  debug_ = debug;

  // Set up VarMap instances for each of the primitive types and their vectors.
  var_map_["bool"] = new VarMap<bool>("bool", this);
  var_map_["int"] = new VarMap<int>("int", this);
  var_map_["double"] = new VarMap<double>("double", this);
  var_map_["string"] = new VarMap<string>("string", this);
  var_map_["bool[]"] = new VarMap<vector<bool> >("bool[]", "bool", this);
  var_map_["int[]"] = new VarMap<vector<int> >("int[]", "int", this);
  var_map_["double[]"] =
      new VarMap<vector<double> >("double[]", "double", this);
  var_map_["string[]"] =
      new VarMap<vector<string> >("string[]", "string", this);

  // Set up VarMap instances for each of the Factory-constructible types
  // and their vectors.
  for (FactoryContainer::iterator factory_it = FactoryContainer::begin();
       factory_it != FactoryContainer::end(); ++factory_it) {
    unordered_set<string> registered;
    (*factory_it)->CollectRegistered(registered);
    string base_name = (*factory_it)->BaseName();

    // Create type-specific VarMap from the Factory and add to var_map_.
    VarMapBase *obj_var_map = (*factory_it)->CreateVarMap(this);
    var_map_[obj_var_map->Name()] = obj_var_map;

    if (debug_ >= 2) {
      cerr << "Environment: created VarMap for " << obj_var_map->Name()
           << endl;
    }

    // Create VarMap for vectors of shared_object of T and add to var_map_.
    VarMapBase *obj_vector_var_map = (*factory_it)->CreateVectorVarMap(this);
    var_map_[obj_vector_var_map->Name()] = obj_vector_var_map;

    if (debug_ >= 2) {
      cerr << "Environment: created VarMap for " << obj_vector_var_map->Name()
           << endl;
    }

    for (unordered_set<string>::const_iterator it = registered.begin();
         it != registered.end(); ++it) {
      const string &concrete_type_name = *it;

      unordered_map<string, string>::const_iterator concrete_to_factory_it =
          concrete_to_factory_type_.find(concrete_type_name);
      if (concrete_to_factory_it != concrete_to_factory_type_.end()) {
        // Warn user that there are two entries for the same concrete type
        // (presumably due to different abstract factory types).
        cerr << "Environment: WARNING: trying to override existing "
             << "concrete-to-factory type mapping ["
             << concrete_type_name << " --> " << concrete_to_factory_it->second
             << "] with [" << concrete_type_name << " --> " << base_name
             << endl;
      }
      concrete_to_factory_type_[concrete_type_name] = base_name;

      if (debug_ >= 2) {
        cerr << "Environment: associating concrete typename "
             << concrete_type_name
             << " with factory for " << base_name << endl;
      }
    }
  }
}

void
EnvironmentImpl::ReadAndSet(const string &varname, StreamTokenizer &st,
                            const string type) {
  bool is_vector =
      st.PeekTokenType() == StreamTokenizer::RESERVED_CHAR &&
      st.Peek() == "{";

  if (is_vector) {
    // Consume open brace.
    st.Next();
  } else if (st.PeekTokenType() == StreamTokenizer::RESERVED_CHAR ||
             (st.PeekTokenType() == StreamTokenizer::RESERVED_WORD &&
              st.Peek() != "true" && st.Peek() != "false" &&
	      st.Peek() != "nullptr" && st.Peek() != "NULL")) {
    ostringstream err_ss;
    err_ss << "Environment: error: expected literal or Factory-constructible "
	   << "type but found token \"" << st.Peek() << "\" of type "
           << StreamTokenizer::TypeName(st.PeekTokenType());
    throw std::runtime_error(err_ss.str());
  }

  string next_tok = st.Peek();
  bool is_object_type = false;

  string inferred_type = InferType(varname, st, is_vector, &is_object_type);

  if (is_vector) {
    st.Putback();
    next_tok = st.Peek();
  }

  if (debug_ >= 1) {
    cerr << "Environment::ReadAndSet: next_tok=\"" << next_tok
         << "\"; explicit type=\"" << type << "\"; "
         << "inferred_type=\"" << inferred_type << "\"" << endl;
  }

  if (type == "" && inferred_type == "") {
    ostringstream err_ss;
    err_ss << "Environment: error: no explicit type specifier and could not "
           << "infer type for variable " << varname;
    throw std::runtime_error(err_ss.str());
  }
  if (type != "" && inferred_type != "" && type != inferred_type) {
    ostringstream err_ss;
    err_ss << "Environment: error: explicit type " << type
           << " and inferred type " << inferred_type
           << " disagree for variable " << varname;
    throw std::runtime_error(err_ss.str());
  }

  // If no explicit type specifier, then the inferred_type is the type.
  string varmap_type = type == "" ? inferred_type : type;

  // Check that varmap_type is a key in var_map_.
  var_map_[varmap_type]->ReadAndSet(varname, st);
  types_[varname] = varmap_type;
}

string
EnvironmentImpl::InferType(const string &varname,
                           const StreamTokenizer &st, bool is_vector,
                           bool *is_object_type) {
  *is_object_type = false;
  string next_tok = st.Peek();
  switch (st.PeekTokenType()) {
    case StreamTokenizer::RESERVED_WORD:
      if (next_tok == "true" || next_tok == "false") {
        return is_vector ? "bool[]" : "bool";
      } else {
        return "";
      }
      break;
    case StreamTokenizer::STRING:
      return is_vector ? "string[]" : "string";
      break;
    case StreamTokenizer::NUMBER:
      {
        // If a token is a NUMBER, it is a double iff it contains a
        // decimal point.
        size_t dot_pos = next_tok.find('.');
        if (dot_pos != string::npos) {
          return is_vector ? "double[]" : "double";
        } else {
          return is_vector ? "int[]" : "int";
        }
      }
      break;
    case StreamTokenizer::IDENTIFIER:
      {
        string type = "";

        // Find out if next_tok is a concrete typename or a variable.
        unordered_map<string, string>::const_iterator factory_type_it =
            concrete_to_factory_type_.find(next_tok);
        unordered_map<string, string>::const_iterator var_type_it =
            types_.find(next_tok);
        if (factory_type_it != concrete_to_factory_type_.end()) {
          // Set type to be abstract factory type.
          if (debug_ >= 1) {
            cerr << "Environment::InferType: concrete type is " << next_tok
                 << "; mapping to abstract Factory type "
                 << factory_type_it->second << endl;
          }
          type = factory_type_it->second;
          *is_object_type = true;
          type = is_vector ? type + "[]" : type;

          if (debug_ >= 1) {
            cerr << "Environment::InferType: type "
                 << (is_vector ? "is" : "isn't")
                 << " a vector, so final inferred type is " << type << endl;
          }
        } else if (var_type_it != types_.end()) {
          // Could be a variable, in which case we need not only to return
          // the variable's type, but also set is_object_type and is_vector
          // based on the variable's type string.
          string append = is_vector ? "[]" : "";
          type = var_type_it->second + append;
          if (debug_ >= 1) {
            cerr << "Environment::InferType: found variable "
                 << var_type_it->first << " of type " << var_type_it->second
                 << "; type is " << type << endl;
          }
        } else {
          ostringstream err_ss;
          err_ss << "Environment: error: token " << next_tok
                 << " is neither a variable nor a concrete object typename";
          throw std::runtime_error(err_ss.str());
        }
        return type;
      }
      break;
    default:
      return "";
  }
  return "";
}

void
EnvironmentImpl::PrintFactories(ostream &os) const {
  FactoryContainer::Print(os);
}

}  // namespace infact
