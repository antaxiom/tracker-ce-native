#ifndef FWREF_SPEC_PARSE_EXTRAS_H_
#define FWREF_SPEC_PARSE_EXTRAS_H_

#include <nlohmann/json.hpp>

namespace fwref
{

struct SpecEntry;

// Parses the optional list/param_list/multi/grid blocks added in wave 1.
void parseSpecExtras(const nlohmann::json& j, SpecEntry& e);

} // namespace fwref

#endif // FWREF_SPEC_PARSE_EXTRAS_H_
