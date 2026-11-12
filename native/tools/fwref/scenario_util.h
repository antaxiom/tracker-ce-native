#ifndef FWREF_SCENARIO_UTIL_H_
#define FWREF_SCENARIO_UTIL_H_

#include "fwref.h"

#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace fwref
{

// Resolves a spec string that is either an integer expression/literal or an
// @scenario.key reference.
int resolveInt(const std::string& exprOrKey, const Scenario& scenario, bool& ok);

// Resolves literal text or @scenario.key.
std::string resolveText(const std::string& literalOrKey, const Scenario& scenario);

// Firmware value formatters (see scenario.cpp provenance comments). `arg`
// selects a per-column value where the firmware label is bound to one control
// (e.g. the five Master EQ bands).
std::string formatLabelValue(const std::string& formatId, const Scenario& scenario,
                             int arg = 0);

// Bar value mapping (gInstrEdit.cpp / gMasterParams.cpp show* functions).
// `arg` selects a track/aux index for the master mixer mappings.
int barValueFor(const std::string& id, int arg, const Scenario& scenario);

// Resolves a scenario list key ("config.baseMenu", "config.items",
// "config.gridNotes", ...). Returns false when the key is unknown.
bool loadScenarioJson(const std::string& path, Scenario& out, std::string& error);

bool resolveStringList(const std::string& key, const Scenario& scenario,
                       std::vector<std::string>& out);
bool resolveRows(const std::string& key, const Scenario& scenario,
                 std::vector<std::pair<std::string, std::string>>& out);

int saturationTypeOf(const Scenario& scenario);
int filterListPosOf(const Scenario& scenario);

// Loader/Sample resolvers and firmware formatters (scenario_sample.cpp).
bool resolveLoaderSampleInt(const std::string& key, const Scenario& scenario, int& out);
std::string resolveLoaderSampleText(const std::string& key, const Scenario& scenario);
std::string formatLoaderSampleValue(const std::string& formatId, const Scenario& scenario);
bool resolveLoaderSampleList(const std::string& key, const Scenario& scenario,
                             std::vector<std::string>& out);
bool resolveLoaderSampleIntList(const std::string& key, const Scenario& scenario,
                                std::vector<int>& out);

// Deterministic representative waveform (no sample data files are read).
void buildSampleWaveform(SampleScenario& sample);
void ensureLoaderSlots(LoaderScenario& loader);

// Scenario JSON blocks (scenario_load_sample.cpp).
void loadLoaderScenarioJson(const nlohmann::json& j, Scenario& out);
void loadSampleScenarioJson(const nlohmann::json& j, Scenario& out);
void loadFileScenarioJson(const nlohmann::json& j, Scenario& out);
void finalizeLoaderSampleScenario(Scenario& out);

} // namespace fwref

#endif // FWREF_SCENARIO_UTIL_H_
