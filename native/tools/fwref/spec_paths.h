#ifndef FWREF_SPEC_PATHS_H_
#define FWREF_SPEC_PATHS_H_

#include <string>

namespace fwref
{

// Returns the directory containing the checked-in fwref specs. Honours the
// TRAKR_FWREF_SPECS environment variable, then the compile-time source path,
// then a few relative candidates (for tools run outside the build dir).
std::string specDirectory();

// Returns the shared comparison scenario (native/tools/fwref/scenario.json).
std::string scenarioFile();

} // namespace fwref

#endif // FWREF_SPEC_PATHS_H_
