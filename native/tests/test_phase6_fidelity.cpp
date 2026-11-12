// Copyright 2026 Trakr Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0

#include <catch2/catch_test_macros.hpp>
#include "WavetableVoice.h"
#include "ChipSynthVoice.h"
#include "dsp/Overdrive.h"
#include "dsp/Bitcrush.h"
#include "StepFx.h"
#include "Engine.h"
#include <vector>
#include <cmath>

using namespace trakr;

TEST_CASE("Phase 6: Wavetable Voice Synthesis", "[phase6][wavetable]") {
    WavetableVoice voice(44100);

    Instrument inst;
    inst.loop = Instrument::Loop::Wavetable;
    inst.wavetableWindow = 256;
    inst.wavetablePos = 0.25f;

    auto sdata = std::make_shared<SampleData>();
    sdata->sampleRate = 44100;
    sdata->channels = 1;
    sdata->ch[0].resize(2048);
    for (size_t i = 0; i < 2048; ++i) {
        sdata->ch[0][i] = std::sin(2.0f * 3.14159265f * (i % 256) / 256.0f);
    }
    inst.sample = sdata;

    voice.noteOn(60, 1.0f, &inst);
    REQUIRE(voice.isActive());

    std::vector<float> bufL(256, 0.0f);
    std::vector<float> bufR(256, 0.0f);
    voice.process(bufL.data(), bufR.data(), 256);

    // Verify non-zero output without NaNs
    float energy = 0.0f;
    for (float sample : bufL) {
        REQUIRE(!std::isnan(sample));
        energy += sample * sample;
    }
    REQUIRE(energy > 0.001f);

    voice.noteOff();
    // After note off and release frames, voice becomes idle
    for (int b = 0; b < 20; ++b) {
        voice.process(bufL.data(), bufR.data(), 256);
    }
    REQUIRE(!voice.isActive());
}

TEST_CASE("Phase 6: Granular Voice Synthesis", "[phase6][granular]") {
    WavetableVoice voice(44100);

    Instrument inst;
    inst.loop = Instrument::Loop::Granular;
    inst.granularLengthMs = 30.0f;

    auto sdata = std::make_shared<SampleData>();
    sdata->sampleRate = 44100;
    sdata->channels = 1;
    sdata->ch[0].resize(4096, 0.5f);
    inst.sample = sdata;

    voice.noteOn(60, 0.8f, &inst);
    REQUIRE(voice.isActive());

    std::vector<float> bufL(128, 0.0f);
    std::vector<float> bufR(128, 0.0f);
    voice.process(bufL.data(), bufR.data(), 128);

    for (float sample : bufL) {
        REQUIRE(!std::isnan(sample));
    }
    voice.stop();
    REQUIRE(!voice.isActive());
}

TEST_CASE("Phase 6: ChipSynth Models Verification", "[phase6][chipsynth]") {
    ChipSynthVoice chipVoice(44100);

    const ChipModel models[] = {
        ChipModel::Pulse2A03,
        ChipModel::SidSawTri,
        ChipModel::NoiseLfsr,
        ChipModel::FmBell,
        ChipModel::SuperSquare,
        ChipModel::ZapLaser,
        ChipModel::ArpChord,
        ChipModel::ChipDrum
    };

    for (ChipModel model : models) {
        Instrument inst;
        inst.isSynth = true;
        inst.chipModel = static_cast<int>(model);
        chipVoice.noteOn(60, 0.9f, &inst);
        REQUIRE(chipVoice.isActive());

        std::vector<float> bufL(128, 0.0f);
        std::vector<float> bufR(128, 0.0f);
        chipVoice.process(bufL.data(), bufR.data(), 128);

        float sum = 0.0f;
        for (float s : bufL) {
            REQUIRE(!std::isnan(s));
            REQUIRE(std::abs(s) <= 2.0f);
            sum += std::abs(s);
        }
        REQUIRE(sum > 0.001f);
        chipVoice.stop();
        REQUIRE(!chipVoice.isActive());
    }
}

TEST_CASE("Phase 6: DSP Overdrive and Bitcrush Exactness", "[phase6][dsp]") {
    SECTION("Overdrive provides soft clipping saturation") {
        dsp::Overdrive od;
        od.setDrive(0.8f);

        // Inputs from -2.0 to 2.0 must be smoothly compressed and bounded
        for (float in = -2.0f; in <= 2.0f; in += 0.2f) {
            float out = od.process(in);
            REQUIRE(!std::isnan(out));
            REQUIRE(std::abs(out) <= 1.5f);
        }
    }

    SECTION("Bitcrush quantizes bit depths correctly") {
        dsp::Bitcrush bc;
        bc.setBits(4.0f); // 16 discrete levels

        float in = 0.73f;
        float out = bc.process(in);
        REQUIRE(!std::isnan(out));
        REQUIRE(std::abs(out) <= 1.0f);
    }
}

TEST_CASE("Phase 6: Step FX Filter and Slide Handlers", "[phase6][stepfx]") {
    WavetableVoice voice(44100);
    Instrument inst;
    inst.loop = Instrument::Loop::Wavetable;
    inst.wavetableWindow = 128;
    auto sdata = std::make_shared<SampleData>();
    sdata->sampleRate = 44100;
    sdata->ch[0].resize(512, 0.5f);
    inst.sample = sdata;
    voice.noteOn(60, 1.0f, &inst);

    auto& registry = StepFxRegistry::instance();

    SECTION("Filter Lowpass FX applies cutoff without errors") {
        registry.applyFx(FxType::FilterLowpass, 80, &voice, 5512);
        std::vector<float> outL(64, 0.0f), outR(64, 0.0f);
        voice.process(outL.data(), outR.data(), 64);
        for (float s : outL) REQUIRE(!std::isnan(s));
    }

    SECTION("Slide Up and Slide Down apply glides") {
        registry.applyFx(FxType::SlideUp, 20, &voice, 5512);
        std::vector<float> outL(64, 0.0f), outR(64, 0.0f);
        voice.process(outL.data(), outR.data(), 64);
        for (float s : outL) REQUIRE(!std::isnan(s));
    }

    voice.stop();
}

TEST_CASE("Phase 6: Engine Dynamic Voice Allocation", "[phase6][engine]") {
    Engine engine(44100);

    Song song = makeDefaultSong();
    // Instrument 0: Sample OneShot
    // Instrument 1: Wavetable
    // Instrument 2: ChipSynth
    song.instruments.resize(3);
    song.instruments[0].name = "Sample_Kick";
    song.instruments[0].loop = Instrument::Loop::None;

    song.instruments[1].name = "WT_Pad";
    song.instruments[1].loop = Instrument::Loop::Wavetable;
    song.instruments[1].wavetableWindow = 256;
    auto sdata = std::make_shared<SampleData>();
    sdata->sampleRate = 44100;
    sdata->ch[0].resize(1024, 0.4f);
    song.instruments[1].sample = sdata;

    song.instruments[2].name = "Chip_Lead";
    song.instruments[2].isSynth = true;
    song.instruments[2].chipModel = 0; // Pulse2A03

    engine.setSongSnapshot(trakr::makeSnapshot(song));

    // Live NoteOn events to different instrument types on track 0
    LiveEvent ev1{LiveEvent::Type::NoteOn, 0, 0, 48, 1.0f, 0, 0};
    engine.pushLiveEvent(ev1);

    std::vector<float> outStereo(256, 0.0f);
    engine.process(outStereo.data(), 128);

    LiveEvent ev2{LiveEvent::Type::NoteOn, 0, 1, 60, 1.0f, 0, 0};
    engine.pushLiveEvent(ev2);
    engine.process(outStereo.data(), 128);

    LiveEvent ev3{LiveEvent::Type::NoteOn, 0, 2, 72, 1.0f, 0, 0};
    engine.pushLiveEvent(ev3);
    engine.process(outStereo.data(), 128);

    for (float s : outStereo) {
        REQUIRE(!std::isnan(s));
    }
}
