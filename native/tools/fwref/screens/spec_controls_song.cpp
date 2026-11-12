#include "spec_controls.h"

#include "fwref.h"
#include "scenario_util.h"
#include "songPlayerControl.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace fwref
{
namespace
{

constexpr uint8_t kNormalNameStride = 21; // fSongEdit.cpp:1008
constexpr uint8_t kMidiNameStride = 8;    // TRACK_NAME_LENGTH + 1 (mtStructs.h:140)
constexpr size_t kMaskBytes = 256;        // PATTERN_INDEX_MAX

std::deque<player_data_t> gPlayers;
std::deque<localList_t> gSongLists;
std::deque<std::vector<uint8_t>> gPlaylists;
std::deque<std::vector<uint8_t>> gNormalMasks;
std::deque<std::vector<uint8_t>> gMidiMasks;
std::deque<std::vector<uint8_t>> gNormalMutes;
std::deque<std::vector<uint8_t>> gMidiMutes;
std::deque<std::string> gNormalNameBuffers;
std::deque<std::string> gMidiNameBuffers;
std::deque<std::vector<char*>> gPatternNamePtrs;
std::deque<uint8_t> gEditModes;

uint8_t clampByte(int value)
{
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return static_cast<uint8_t>(value);
}

void fillTrackNameBuffer(std::string& buffer, const std::array<std::string, 16>& names, int first,
                         int count, uint8_t stride)
{
    buffer.assign(static_cast<size_t>(count) * stride, '\0');
    for (int i = 0; i < count; ++i)
    {
        const int source = first + i;
        if (source < 0 || source >= static_cast<int>(names.size()))
            continue;
        const std::string& name = names[static_cast<size_t>(source)];
        const size_t maxLen = static_cast<size_t>(stride) - 1;
        std::memcpy(&buffer[static_cast<size_t>(i) * stride], name.c_str(),
                    std::min(name.size(), maxLen));
    }
}

} // namespace

void resetSongSpecArenas()
{
    gPlayers.clear();
    gSongLists.clear();
    gPlaylists.clear();
    gNormalMasks.clear();
    gMidiMasks.clear();
    gNormalMutes.clear();
    gMidiMutes.clear();
    gNormalNameBuffers.clear();
    gMidiNameBuffers.clear();
    gPatternNamePtrs.clear();
    gEditModes.clear();
}

hControl createSongPlayerControl(const SpecEntry& e, strControlProperties& prop,
                                 const Scenario& scenario, bool& ok)
{
    const SongScenario& song = scenario.song;

    std::vector<uint8_t> playlist;
    playlist.reserve(song.playlist.size());
    for (int value : song.playlist)
        playlist.push_back(clampByte(value));
    if (playlist.empty())
        playlist.push_back(1);

    int length = static_cast<int>(playlist.size());
    if (e.hasSong && !e.song.length.empty())
    {
        int overrideLength = resolveInt(e.song.length, scenario, ok);
        if (!ok)
            return static_cast<hControl>(-1);
        length = std::min(overrideLength, static_cast<int>(playlist.size()));
    }
    length = std::max(0, length);

    int start = song.selectedSlot;
    if (e.hasSong && !e.song.start.empty())
        start = resolveInt(e.song.start, scenario, ok);
    if (!ok)
        return static_cast<hControl>(-1);
    start = (length > 0) ? std::clamp(start, 0, length - 1) : 0;

    gPlaylists.push_back(std::move(playlist));
    std::vector<uint8_t>& songs = gPlaylists.back();

    gNormalMasks.emplace_back(kMaskBytes, 0);
    gMidiMasks.emplace_back(kMaskBytes, 0);
    std::vector<uint8_t>& normalMasks = gNormalMasks.back();
    std::vector<uint8_t>& midiMasks = gMidiMasks.back();
    for (size_t i = 0; i < song.normalPatternMask.size() && i < kMaskBytes; ++i)
        normalMasks[i] = clampByte(song.normalPatternMask[i]);
    for (size_t i = 0; i < song.midiPatternMask.size() && i < kMaskBytes; ++i)
        midiMasks[i] = clampByte(song.midiPatternMask[i]);

    gNormalMutes.emplace_back(8, 0);
    gMidiMutes.emplace_back(8, 0);
    std::vector<uint8_t>& normalMutes = gNormalMutes.back();
    std::vector<uint8_t>& midiMutes = gMidiMutes.back();
    for (size_t i = 0; i < song.normalMuted.size(); ++i)
        normalMutes[i] = song.normalMuted[i] ? 1 : 0;
    for (size_t i = 0; i < song.midiMuted.size(); ++i)
        midiMutes[i] = song.midiMuted[i] ? 1 : 0;

    gNormalNameBuffers.emplace_back();
    gMidiNameBuffers.emplace_back();
    std::string& normalNames = gNormalNameBuffers.back();
    std::string& midiNames = gMidiNameBuffers.back();
    fillTrackNameBuffer(normalNames, song.trackNames, 0, song.normalTracksCount,
                        kNormalNameStride);
    fillTrackNameBuffer(midiNames, song.trackNames, song.normalTracksCount, song.midiTracksCount,
                        kMidiNameStride);

    gPatternNamePtrs.emplace_back();
    std::vector<char*>& namePtrs = gPatternNamePtrs.back();
    bool hasNames = !song.patternNames.empty();
    if (hasNames)
    {
        namePtrs.reserve(static_cast<size_t>(length));
        for (int i = 0; i < length; ++i)
        {
            static const std::string kEmpty;
            const std::string& name =
              i < static_cast<int>(song.patternNames.size()) ? song.patternNames[i] : kEmpty;
            namePtrs.push_back(const_cast<char*>(name.c_str()));
        }
    }

    gSongLists.emplace_back();
    localList_t& list = gSongLists.back();
    list.start = static_cast<int16_t>(start);
    list.length = static_cast<uint16_t>(length);
    list.linesCount = static_cast<uint16_t>(e.hasSong ? e.song.linesCount : 12);
    list.data = songs.data();
    list.names = hasNames ? namePtrs.data() : nullptr;
    list.icon = text_icon_t{};
    list.icon.iconPositionInList = 0;

    gEditModes.push_back(song.editMode ? 1 : 0);
    gPlayers.push_back(player_data_t{});
    player_data_t& pd = gPlayers.back();
    pd.songLength = clampByte(length);
    pd.songData = songs.data();
    pd.isEditMode = &gEditModes.back();
    pd.isListActive = static_cast<uint8_t>(song.isListActive ? 1 : 0);
    pd.selection.isActive = static_cast<uint8_t>(song.selectionIsActive ? 1 : 0);
    pd.selection.startSongPosition = clampByte(song.selectionStartSongPosition);
    pd.selection.patternSelectionLength = clampByte(song.selectionPatternLength);
    pd.selection.startTrack = clampByte(song.selectionStartTrack);
    pd.selection.trackSelectionLength = clampByte(song.selectionTrackLength);
    pd.progress.isPlaying = static_cast<uint8_t>(song.progressIsPlaying ? 1 : 0);
    pd.progress.trackPreview = clampByte(song.progressTrackPreview);
    pd.progress.currentSongPosition = clampByte(song.progressCurrentSongPosition);
    pd.progress.patternLength = clampByte(song.progressPatternLength);
    pd.progress.positionInPattern = clampByte(song.progressPositionInPattern);
    pd.list = &list;
    pd.normalTracksCount = clampByte(song.normalTracksCount);
    pd.midiTracksCount = clampByte(song.midiTracksCount);
    pd.normalPatternsBitmask = normalMasks.data();
    pd.midiPatternsBitmask = midiMasks.data();
    pd.normalTracksMute = normalMutes.data();
    pd.midiTracksMute = midiMutes.data();
    pd.normalTracksNames = normalNames.data();
    pd.midiTracksNames = midiNames.data();
    pd.normalTracksNamesBuffLen = kNormalNameStride;
    pd.midiTracksNamesBuffLen = kMidiNameStride;

    prop.data = nullptr;
    hControl handle = display.createControl<cSongPlayer>(&prop);
    if (handle >= 0)
        display.setControlData(handle, &pd);
    return handle;
}

} // namespace fwref
