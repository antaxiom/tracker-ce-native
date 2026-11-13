#include "Song.h"

namespace trakr {

Song makeDefaultSong() {
    Song song;
    song.bpm = 120.0f;
    song.lpb = 4;

    constexpr int kDefaultTracks = 8;
    constexpr int kDefaultRows = 64;

    song.tracks.resize(kDefaultTracks);
    for (int i = 0; i < kDefaultTracks; ++i) {
        song.tracks[i].name = "Track " + std::to_string(i + 1);
    }

    Pattern p;
    p.rows = kDefaultRows;
    p.tracks.resize(kDefaultTracks);
    for (int t = 0; t < kDefaultTracks; ++t) {
        p.tracks[t].resize(kDefaultRows);
    }
    song.patterns.push_back(p);
    song.order = {0};

    Instrument inst;
    inst.name = "Init Instrument";
    song.instruments.push_back(inst);

    return song;
}

std::shared_ptr<const SongSnapshot> makeSnapshot(const Song& song) {
    auto snapshot = std::make_shared<SongSnapshot>();
    snapshot->bpm = song.bpm;
    snapshot->lpb = song.lpb;
    snapshot->tracks = song.tracks;
    snapshot->patterns = song.patterns;
    snapshot->order = song.order;
    snapshot->instruments = song.instruments;
    snapshot->masterGainDb = song.masterGainDb;
    snapshot->masterPan = song.masterPan;
    snapshot->delayTimeMs = song.delayTimeMs;
    snapshot->delayFeedback = song.delayFeedback;
    snapshot->reverbSize = song.reverbSize;
    snapshot->reverbDamping = song.reverbDamping;
    snapshot->masterBassBoost = song.masterBassBoost;
    snapshot->masterSaturationDrive = song.masterSaturationDrive;
    snapshot->masterSaturationType = song.masterSaturationType;
    snapshot->masterSpace = song.masterSpace;
    for (int i = 0; i < 5; ++i) snapshot->masterEq[i] = song.masterEq[i];
    snapshot->headroom = song.headroom;
    for (int i = 0; i < 4; ++i) {
        snapshot->auxVolume[i] = song.auxVolume[i];
        snapshot->auxMute[i] = song.auxMute[i];
    }
    return snapshot;
}

} // namespace trakr

