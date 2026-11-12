#include "fwref.h"

#include <cstdio>
#include <string>

int main()
{
    std::string error;
    if (!fwref::runOracleSelfTest(error))
    {
        std::fprintf(stderr, "fwref_selftest FAILED: %s\n", error.c_str());
        return 1;
    }
    std::printf("fwref_selftest OK\n");
    return 0;
}
