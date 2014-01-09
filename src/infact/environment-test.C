/// \file
/// Test driver for the Environment class.
/// \author dbikel@google.com (Dan Bikel)

#include <iostream>
#include <memory>

#include "environment-impl.H"

using namespace std;
using namespace infact;

int
main(int argc, char **argv) {
  int debug = 1;
  Environment *env = new EnvironmentImpl(debug);
  delete env;
}
