#include "DemoSong.h"
#include <cmath>

namespace trakr {

void populateDemoSong(Song& song) {
    song.bpm = 126.0f;
    song.lpb = 4;
    song.masterGainDb = 0.0f; // Firmware default MASTER_VOLUME_DEFAULT (81 -> 0.00 dB)

    // 1. Configure 8 Dedicated Tracks with Authentic Gain Staging
    song.tracks.resize(8);
    song.tracks[0].name = "Kick";  song.tracks[0].gainDb = -3.0f;
    song.tracks[1].name = "Snare"; song.tracks[1].gainDb = -3.5f;
    song.tracks[2].name = "HiHat"; song.tracks[2].gainDb = -5.0f;
    song.tracks[3].name = "Bass";  song.tracks[3].gainDb = -3.5f;
    song.tracks[4].name = "Lead";  song.tracks[4].gainDb = -5.0f;
    song.tracks[5].name = "Chip";  song.tracks[5].gainDb = -5.5f;
    song.tracks[6].name = "Arp";   song.tracks[6].gainDb = -5.5f;
    song.tracks[7].name = "Noise"; song.tracks[7].gainDb = -6.0f;

    // 2. Configure 8 Distinct Instruments (Samples + Phase 6 Chip Synths)
    song.instruments.clear();

    // Inst 0: "909 Kick" (1.200s reference test sample, 52920 frames at 44.1kHz)
    Instrument instKick;
    instKick.name = "909 Kick";
    auto sKick = std::make_shared<SampleData>();
    sKick->sampleRate = 44100;
    sKick->channels = 1;
    sKick->ch[0].resize(52920);
    for (size_t i = 0; i < sKick->ch[0].size(); ++i) {
        float norm = static_cast<float>(i) / 52920.0f;
        sKick->ch[0][i] = std::sin(norm * 45.0f) * std::exp(-norm * 3.0f);
    }
    instKick.sample = sKick;
    instKick.filterType = Instrument::FilterType::Lowpass;
    instKick.slices = {0, 13230, 26460, 39690};
    song.instruments.push_back(instKick);

    // Inst 1: "909 Snare Snappy"
    Instrument instSnare;
    instSnare.name = "909 Snare";
    auto sSnare = std::make_shared<SampleData>();
    sSnare->sampleRate = 44100;
    sSnare->channels = 1;
    sSnare->ch[0].resize(static_cast<size_t>(44100 * 0.28f));
    uint32_t seed = 0x12345678;
    for (size_t i = 0; i < sSnare->ch[0].size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        float tone = std::sin(2.0f * 3.14159265f * 185.0f * t) * std::exp(-t * 28.0f);
        seed = seed * 1664525u + 1013904223u;
        float noise = (static_cast<float>(seed >> 16) / 32768.0f - 1.0f);
        sSnare->ch[0][i] = (tone * 0.45f + noise * std::exp(-t * 16.0f) * 0.55f);
    }
    instSnare.sample = sSnare;
    song.instruments.push_back(instSnare);

    // Inst 2: "909 Closed Hat"
    Instrument instHat;
    instHat.name = "909 HiHat";
    auto sHat = std::make_shared<SampleData>();
    sHat->sampleRate = 44100;
    sHat->channels = 1;
    sHat->ch[0].resize(static_cast<size_t>(44100 * 0.08f));
    for (size_t i = 0; i < sHat->ch[0].size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        seed = seed * 1664525u + 1013904223u;
        float noise = (static_cast<float>(seed >> 16) / 32768.0f - 1.0f);
        sHat->ch[0][i] = noise * std::exp(-t * 55.0f) * 0.7f;
    }
    instHat.sample = sHat;
    song.instruments.push_back(instHat);

    // Inst 3: "Acid 303 Bass"
    Instrument instBass;
    instBass.name = "Acid 303";
    auto sBass = std::make_shared<SampleData>();
    sBass->sampleRate = 44100;
    sBass->channels = 1;
    sBass->ch[0].resize(44100 / 2);
    for (size_t i = 0; i < sBass->ch[0].size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        float phase = std::fmod(t * 110.0f, 1.0f);
        float saw = 0.0f;
        for (int h = 1; h <= 16; ++h) saw += (std::sin(2.0f * 3.14159265f * h * phase) / h);
        sBass->ch[0][i] = saw * (2.0f / 3.14159265f) * 0.65f;
    }
    instBass.sample = sBass;
    instBass.rootNote = 45;
    instBass.loop = Instrument::Loop::Forward;
    instBass.loopStart = 0;
    instBass.loopEnd = static_cast<int>(sBass->ch[0].size());
    instBass.attackMs = 1.0f;
    instBass.decayMs = 240.0f;
    instBass.sustain = 0.0f; // Decays naturally: no endless drone
    instBass.releaseMs = 30.0f;
    instBass.filterType = Instrument::FilterType::Lowpass;
    instBass.filterCutoffHz = 1800.0f;
    instBass.filterResonance = 2.8f;
    song.instruments.push_back(instBass);

    // Inst 4: "Poly Lead Synth"
    Instrument instLead;
    instLead.name = "Poly Lead";
    auto sLead = std::make_shared<SampleData>();
    sLead->sampleRate = 44100;
    sLead->channels = 1;
    sLead->ch[0].resize(44100 / 2);
    for (size_t i = 0; i < sLead->ch[0].size(); ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        float p1 = std::fmod(t * 220.0f, 1.0f);
        float p2 = std::fmod(t * 221.4f, 1.0f);
        sLead->ch[0][i] = (((p1 < 0.5f ? 0.4f : -0.4f) + (p2 < 0.5f ? 0.4f : -0.4f)) * 0.5f);
    }
    instLead.sample = sLead;
    instLead.rootNote = 57;
    instLead.loop = Instrument::Loop::Forward;
    instLead.loopStart = 0;
    instLead.loopEnd = static_cast<int>(sLead->ch[0].size());
    instLead.attackMs = 2.0f;
    instLead.decayMs = 320.0f;
    instLead.sustain = 0.0f; // Decays naturally after trigger
    instLead.releaseMs = 40.0f;
    instLead.filterType = Instrument::FilterType::Lowpass;
    instLead.filterCutoffHz = 4200.0f;
    instLead.filterResonance = 1.2f;
    song.instruments.push_back(instLead);

    // Inst 5: "Chip 2A03 Pulse" (Phase 6 Chip Synth)
    Instrument instChipPulse;
    instChipPulse.name = "2A03 Pulse";
    instChipPulse.isSynth = true;
    instChipPulse.chipModel = 0; // Pulse2A03
    instChipPulse.attackMs = 1.0f;
    instChipPulse.decayMs = 150.0f;
    instChipPulse.sustain = 0.0f;
    instChipPulse.releaseMs = 25.0f;
    song.instruments.push_back(instChipPulse);

    // Inst 6: "Chip SID Arp" (Phase 6 Chip Synth Arp)
    Instrument instChipArp;
    instChipArp.name = "SID Arp";
    instChipArp.isSynth = true;
    instChipArp.chipModel = 6; // ArpChord
    instChipArp.attackMs = 2.0f;
    instChipArp.decayMs = 240.0f;
    instChipArp.sustain = 0.0f;
    instChipArp.releaseMs = 30.0f;
    song.instruments.push_back(instChipArp);

    // Inst 7: "Chip LFSR Noise" (Phase 6 Chip Synth Noise)
    Instrument instChipNoise;
    instChipNoise.name = "LFSR Noise";
    instChipNoise.isSynth = true;
    instChipNoise.chipModel = 2; // NoiseLfsr
    instChipNoise.attackMs = 1.0f;
    instChipNoise.decayMs = 45.0f;
    instChipNoise.sustain = 0.0f; // Crisp percussive hit
    instChipNoise.releaseMs = 15.0f;
    song.instruments.push_back(instChipNoise);

    // 3. Multi-Pattern Song Setup
    song.patterns.clear();
    song.patterns.resize(3);
    for (int p = 0; p < 3; ++p) {
        song.patterns[p].rows = 64;
        song.patterns[p].tracks.resize(8, std::vector<Cell>(64));
    }
    song.order = {0}; // Initial sequence slot (matching firmware project default)

    // PATTERN 0 (Intro / Groove)
    auto& pat0 = song.patterns[0];
    for (int r = 0; r < 64; r += 4) { // Kick
        pat0.tracks[0][r].note = 48; // C-3
        pat0.tracks[0][r].instrument = 0;
        pat0.tracks[0][r].volume = 56;
    }
    for (int r = 4; r < 64; r += 8) { // Snare
        pat0.tracks[1][r].note = 48;
        pat0.tracks[1][r].instrument = 1;
        pat0.tracks[1][r].volume = 52;
    }
    for (int r = 2; r < 64; r += 4) { // Offbeat Hats
        pat0.tracks[2][r].note = 58;
        pat0.tracks[2][r].instrument = 2;
        pat0.tracks[2][r].volume = 42;
    }
    const int bassNotes[] = {38, 38, 41, 43, 38, 45, 48, 43};
    for (int i = 0; i < 16; ++i) { // Bass
        int r = i * 4;
        pat0.tracks[3][r].note = static_cast<int16_t>(bassNotes[i % 8]);
        pat0.tracks[3][r].instrument = 3;
        pat0.tracks[3][r].volume = 48;
    }

    // PATTERN 1 (Full Peak Arrangement with Chip & Leads)
    auto& pat1 = song.patterns[1];
    for (int r = 0; r < 64; r += 4) { // 4-on-the-floor Kick
        pat1.tracks[0][r].note = 48;
        pat1.tracks[0][r].instrument = 0;
        pat1.tracks[0][r].volume = 56;
    }
    for (int r = 4; r < 64; r += 8) { // Snare + Clap
        pat1.tracks[1][r].note = 48;
        pat1.tracks[1][r].instrument = 1;
        pat1.tracks[1][r].volume = 52;
    }
    for (int r = 0; r < 64; r += 2) { // 16th Hi-Hats
        pat1.tracks[2][r].note = 58;
        pat1.tracks[2][r].instrument = 2;
        pat1.tracks[2][r].volume = (r % 4 == 0) ? 36 : 46;
    }
    for (int i = 0; i < 16; ++i) { // Pumping Bassline
        int r = i * 2;
        pat1.tracks[3][r].note = static_cast<int16_t>(bassNotes[i % 8]);
        pat1.tracks[3][r].instrument = 3;
        pat1.tracks[3][r].volume = 48;
    }
    const int leadMelody[] = {60, 63, 67, 70, 72, 70, 67, 63};
    for (int i = 0; i < 16; ++i) { // Poly Lead
        int r = i * 4 + 1;
        pat1.tracks[4][r].note = static_cast<int16_t>(leadMelody[i % 8]);
        pat1.tracks[4][r].instrument = 4;
        pat1.tracks[4][r].volume = 44;
    }
    for (int i = 0; i < 16; ++i) { // Phase 6 Chip Synth Pulse
        int r = i * 4 + 2;
        pat1.tracks[5][r].note = static_cast<int16_t>(leadMelody[(i + 4) % 8]);
        pat1.tracks[5][r].instrument = 5;
        pat1.tracks[5][r].volume = 40;
    }
    for (int r = 0; r < 64; r += 8) { // Phase 6 Chip SID Arp Chords
        pat1.tracks[6][r].note = static_cast<int16_t>(60 + (r % 12));
        pat1.tracks[6][r].instrument = 6;
        pat1.tracks[6][r].volume = 42;
    }
    for (int r = 0; r < 64; r += 16) { // Noise accents with clean note-offs
        pat1.tracks[7][r].note = 60;
        pat1.tracks[7][r].instrument = 7;
        pat1.tracks[7][r].volume = 32;
        if (r + 2 < 64) pat1.tracks[7][r + 2].note = kNoteOff;
    }

    // Explicit note-offs at end of pattern 1 to prevent hanging voices across boundaries
    pat1.tracks[3][62].note = kNoteOff;
    pat1.tracks[4][62].note = kNoteOff;
    pat1.tracks[5][62].note = kNoteOff;
    pat1.tracks[6][62].note = kNoteOff;

    // PATTERN 2 (Breakdown)
    auto& pat2 = song.patterns[2];
    for (int r = 0; r < 64; r += 8) {
        pat2.tracks[0][r].note = 48;
        pat2.tracks[0][r].instrument = 0;
        pat2.tracks[0][r].volume = 52;
    }
    for (int r = 0; r < 64; r += 4) {
        pat2.tracks[6][r].note = static_cast<int16_t>(62 + (r % 7));
        pat2.tracks[6][r].instrument = 6;
        pat2.tracks[6][r].volume = 44;
    }
    pat2.tracks[6][62].note = kNoteOff;
}

} // namespace trakr
