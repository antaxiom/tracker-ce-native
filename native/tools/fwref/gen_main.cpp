#include "fwref.h"
#include "spec_paths.h"

#include "FT812Emulator.h"
#include "audit.h"

#include <cstdio>
#include <filesystem>
#include <string>

namespace
{

int usage()
{
    std::fprintf(stderr,
                 "usage: fwref_gen [--out DIR] [--scenario FILE] [--sample-state forward|slice|granular] [--strict]\n"
                 "       fwref_gen --selftest\n");
    return 2;
}

} // namespace

int main(int argc, char** argv)
{
    std::string outDir = "render/fwref";
    std::string scenarioPath = fwref::scenarioFile();
    std::string sampleState = "forward";
    bool strict = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--out" && i + 1 < argc)
            outDir = argv[++i];
        else if (a == "--scenario" && i + 1 < argc)
            scenarioPath = argv[++i];
        else if (a == "--sample-state" && i + 1 < argc)
            sampleState = argv[++i];
        else if (a == "--strict")
            strict = true;
        else if (a == "--selftest")
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
        else
            return usage();
    }

    fwref::setStrict(strict);

    fwref::Scenario scenario;
    std::string error;
    if (!fwref::loadScenarioJson(scenarioPath, scenario, error))
    {
        std::fprintf(stderr, "fwref_gen: %s\n", error.c_str());
        return 1;
    }

    std::filesystem::create_directories(outDir);

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderInstrumentGeneral(emu, scenario.instrument))
        {
            std::fprintf(stderr, "fwref_gen: failed to render instrument\n");
            return 1;
        }
        const std::string path = outDir + "/instrument_params.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderMasterPage1(emu, scenario.master))
        {
            std::fprintf(stderr, "fwref_gen: failed to render master\n");
            return 1;
        }
        const std::string path = outDir + "/master_page1.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderPattern(emu, scenario.pattern))
        {
            std::fprintf(stderr, "fwref_gen: failed to render pattern\n");
            return 1;
        }
        const std::string path = outDir + "/pattern.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderInstrumentEffects(emu, scenario.instrument))
        {
            std::fprintf(stderr, "fwref_gen: failed to render instrument effects\n");
            return 1;
        }
        const std::string path = outDir + "/instrument_effects.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderInstrumentAutomation(emu, scenario.instrument))
        {
            std::fprintf(stderr, "fwref_gen: failed to render instrument automation\n");
            return 1;
        }
        const std::string path = outDir + "/instrument_automation.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderMasterPage2(emu, scenario.master))
        {
            std::fprintf(stderr, "fwref_gen: failed to render master page 2\n");
            return 1;
        }
        const std::string path = outDir + "/master_page2.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderMasterPage3(emu, scenario.master))
        {
            std::fprintf(stderr, "fwref_gen: failed to render master page 3\n");
            return 1;
        }
        const std::string path = outDir + "/master_page3.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderMasterEq(emu, scenario.master))
        {
            std::fprintf(stderr, "fwref_gen: failed to render master eq\n");
            return 1;
        }
        const std::string path = outDir + "/master_eq.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderConfig(emu, scenario.config))
        {
            std::fprintf(stderr, "fwref_gen: failed to render config\n");
            return 1;
        }
        const std::string path = outDir + "/config.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderConfigEdit(emu, scenario.config))
        {
            std::fprintf(stderr, "fwref_gen: failed to render config edit\n");
            return 1;
        }
        const std::string path = outDir + "/config_edit.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderPerform(emu, scenario.perform))
        {
            std::fprintf(stderr, "fwref_gen: failed to render perform\n");
            return 1;
        }
        const std::string path = outDir + "/perform.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderSong(emu, scenario.song))
        {
            std::fprintf(stderr, "fwref_gen: failed to render song\n");
            return 1;
        }
        const std::string path = outDir + "/song.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderLoader(emu, scenario.loader))
        {
            std::fprintf(stderr, "fwref_gen: failed to render loader\n");
            return 1;
        }
        const std::string path = outDir + "/loader.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    if (sampleState == "slice")
    {
        scenario.sample.playMode = 4; // playModeSlice
    }
    else if (sampleState == "granular")
    {
        // playModeGranular shows the cLineIndicator grain window. The values
        // mirror the demo project the app renders for the sample_granular
        // parity case: playhead at spectrum position 0 with a ~42-frame grain
        // (gSamplePlay showGranularPositionValue() widens the half window to a
        // [0, 22) column span at the left sample edge).
        scenario.sample.playMode = 7; // playModeGranular
        scenario.sample.progress = 0;
        scenario.sample.regionStart = 0;
        scenario.sample.regionEnd = 22;
    }
    else if (sampleState != "forward")
    {
        std::fprintf(stderr, "fwref_gen: unknown --sample-state %s\n", sampleState.c_str());
        return usage();
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        const std::string specName =
            sampleState == "slice" ? "sample_slice.json"
            : sampleState == "granular" ? "sample_granular.json"
            : "sample.json";
        if (!fwref::renderSampleState(emu, scenario.sample, specName))
        {
            std::fprintf(stderr, "fwref_gen: failed to render sample\n");
            return 1;
        }
        const std::string path = outDir + "/sample" +
                                 (sampleState == "forward" ? std::string()
                                                           : "_" + sampleState) +
                                 ".png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    {
        trakr::emulator::FT812DisplayEmulator emu;
        if (!fwref::renderFile(emu, scenario.file))
        {
            std::fprintf(stderr, "fwref_gen: failed to render file\n");
            return 1;
        }
        const std::string path = outDir + "/file.png";
        if (!emu.savePng(path))
        {
            std::fprintf(stderr, "fwref_gen: cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", path.c_str());
    }

    fwref::printAuditReport("fwref_gen");
    if (strict && fwref::auditFailed())
    {
        std::fprintf(stderr, "fwref_gen: strict mode failed with %d unsupported call(s)\n",
                     fwref::auditFailures());
        return 1;
    }
    return 0;
}
