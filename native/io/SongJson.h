#pragma once

#include "Song.h"
#include <string>

namespace trakr {

bool saveSongJson(const Song& song, const std::string& filepath);
bool loadSongJson(Song& outSong, const std::string& filepath);

} // namespace trakr

