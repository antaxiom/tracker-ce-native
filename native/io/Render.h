#pragma once

#include "Song.h"
#include <string>

namespace trakr {

bool renderToWav(const Song& song,
                 const std::string& filepath,
                 int sampleRate = 44100,
                 int bitDepth = 16);

bool renderStemsToWav(const Song& song,
                      const std::string& outputDir,
                      int sampleRate = 44100,
                      int bitDepth = 16);

} // namespace trakr

