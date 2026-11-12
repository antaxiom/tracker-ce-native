#pragma once

#include "Song.h"
#include <string>

namespace trakr::io {

// Loads a standard audio file (.wav, .flac, etc.) into SampleData
bool loadAudioFile(const std::string& path, SampleData& outSample);

// Unpacks a Polyend .pti instrument file (392-byte header + PCM payload)
bool loadPtiInstrument(const std::string& path, Instrument& outInst);

// Loads a Polyend .mtp pattern file into a trakr Pattern
bool loadMtpPattern(const std::string& path, Pattern& outPat);

// Loads a Polyend project folder (.mt) or directory containing patterns and instruments
bool loadPolyendProject(const std::string& projectDirOrFile, Song& outSong);

} // namespace trakr::io

