#include "spec_paths.h"

#include <cstdlib>
#include <filesystem>

namespace fwref
{

std::string specDirectory()
{
    const char* env = std::getenv("TRAKR_FWREF_SPECS");
    if (env != nullptr && std::filesystem::exists(env))
        return env;

#ifdef FWREF_SPEC_DIR
    if (std::filesystem::exists(FWREF_SPEC_DIR))
        return FWREF_SPEC_DIR;
#endif

    namespace fs = std::filesystem;
    const std::string candidates[] = {
        "native/tools/fwref/specs",
        "../native/tools/fwref/specs",
        "tools/fwref/specs",
        "specs",
        "../../native/tools/fwref/specs",
    };
    for (const auto& c : candidates)
    {
        if (fs::exists(c))
            return c;
    }
    return "native/tools/fwref/specs";
}

std::string scenarioFile()
{
    namespace fs = std::filesystem;
    const fs::path dir = specDirectory();
    const fs::path file = dir.parent_path() / "scenario.json";
    if (fs::exists(file))
        return file.string();
    return (dir / "scenario.json").string();
}

} // namespace fwref
