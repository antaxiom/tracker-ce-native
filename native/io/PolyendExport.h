#pragma once

#include "Song.h"
#include <string>

namespace trakr::io {

// Exports an instrument to a Polyend Tracker .pti file (16-bit PCM + metadata header)
bool exportPtiInstrument(const std::string& path, const Instrument& inst);

// Exports a pattern to a Polyend Tracker .mtp file (8 tracks, up to 128 steps)
bool exportMtpPattern(const std::string& path, const Pattern& pat);

// Exports a complete song to a Polyend Tracker project folder (.mt + patterns + instruments)
bool exportPolyendProject(const std::string& projectDir, const Song& song);

} // namespace trakr::io

