#include "KitGallery.h"
#include "Widgets.h"
#include <vector>
#include <cmath>

namespace trakr::kit {

void renderKitGallery(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(780, 560), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Trakr Widget Kit Gallery (Renoise Aesthetic)", p_open)) {
        ImGui::End();
        return;
    }

    PanelHeader("TRANSPORT & BUTTON CONTROLS", "PHASE 3");
    ImGui::Spacing();

    static bool playing = false;
    static bool recording = false;
    if (FlatButton(playing ? "PAUSE" : "PLAY", ImVec2(70, 24), playing)) {
        playing = !playing;
    }
    ImGui::SameLine();
    if (FlatButton("STOP", ImVec2(60, 24))) {
        playing = false;
    }
    ImGui::SameLine();
    if (ToggleButton("REC", &recording, ImVec4(0.85f, 0.2f, 0.2f, 1.0f), ImVec2(60, 24))) {
        // toggled recording
    }

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    static bool mute1 = false;
    static bool solo1 = false;
    ToggleButton("MUTE", &mute1, ImVec4(0.8f, 0.45f, 0.1f, 1.0f), ImVec2(50, 24));
    ImGui::SameLine();
    ToggleButton("SOLO", &solo1, ImVec4(0.85f, 0.75f, 0.15f, 1.0f), ImVec2(50, 24));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    PanelHeader("ROTARY KNOBS & NUMERIC VALUE BOXES");
    ImGui::Spacing();

    static float cutoff = 1200.0f;
    static float resonance = 0.45f;
    static float pan = 0.0f;
    static float masterVol = 0.85f;

    Knob("Cutoff", &cutoff, 20.0f, 20000.0f, "%.0f Hz");
    ImGui::SameLine(0, 16);
    Knob("Reso", &resonance, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine(0, 16);
    Knob("Pan", &pan, -1.0f, 1.0f, "%.2f");
    ImGui::SameLine(0, 16);
    Knob("Master", &masterVol, 0.0f, 1.0f, "%.2f");

    ImGui::SameLine(0, 30);
    ImGui::BeginGroup();
    ImGui::TextDisabled("ValueBoxes:");
    static int hexNote = 0x3C; // 60 (C-4)
    static int hexInst = 0x01;
    static int hexVol = 0x40;  // 64
    static int bpmVal = 135;

    ImGui::Text("Note (Hex):");
    ImGui::SameLine();
    ValueBox("##NoteHex", &hexNote, 0, 127, true);

    ImGui::Text("Inst (Hex):");
    ImGui::SameLine();
    ValueBox("##InstHex", &hexInst, 0, 255, true);

    ImGui::Text("Vol  (Hex):");
    ImGui::SameLine();
    ValueBox("##VolHex", &hexVol, 0, 64, true);

    ImGui::Text("BPM  (Dec):");
    ImGui::SameLine();
    ValueBox("##BpmDec", &bpmVal, 20, 999, false);
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    PanelHeader("MINISLIDERS & AUDIO PEAK METERS");
    ImGui::Spacing();

    static float delaySend = 0.25f;
    static float reverbSend = 0.40f;
    static float trackFader = 0.80f;

    ImGui::BeginGroup();
    ImGui::Text("Delay Send:");
    MiniSlider("##DelaySend", &delaySend, 0.0f, 1.0f, ImVec2(120, 18));
    ImGui::Text("Reverb Send:");
    MiniSlider("##ReverbSend", &reverbSend, 0.0f, 1.0f, ImVec2(120, 18));
    ImGui::EndGroup();

    ImGui::SameLine(0, 30);
    ImGui::BeginGroup();
    ImGui::Text("Track Fader:");
    MiniSliderV("##TrackFader", &trackFader, 0.0f, 1.0f, ImVec2(20, 75));
    ImGui::EndGroup();

    ImGui::SameLine(0, 20);
    ImGui::BeginGroup();
    ImGui::Text("Track 1:");
    Meter(0.72f, 0.85f, ImVec2(8, 75), true);
    ImGui::SameLine(0, 4);
    Meter(0.68f, 0.82f, ImVec2(8, 75), true);
    ImGui::EndGroup();

    ImGui::SameLine(0, 20);
    ImGui::BeginGroup();
    ImGui::Text("Master:");
    Meter(0.91f, 0.98f, ImVec2(10, 75), true);
    ImGui::SameLine(0, 4);
    Meter(0.89f, 0.97f, ImVec2(10, 75), true);
    ImGui::EndGroup();

    ImGui::SameLine(0, 30);
    ImGui::BeginGroup();
    static int currentInst = 0;
    static std::vector<std::string> instList = {
        "00: 909 Kick Heavy",
        "01: 909 Snare Crisp",
        "02: Closed HiHat 01",
        "03: Open HiHat Long",
        "04: TB-303 Acid Saw",
        "05: SH-101 Sub Bass"
    };
    ImGui::Text("Instrument Slots:");
    ListBox("##InstList", &currentInst, instList, 4, 180.0f);
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    PanelHeader("WAVEFORM VIEW & SAMPLE MARKERS");
    ImGui::Spacing();

    static std::vector<float> sampleData;
    if (sampleData.empty()) {
        sampleData.resize(512);
        for (size_t i = 0; i < sampleData.size(); ++i) {
            float t = static_cast<float>(i) / sampleData.size();
            float env = std::exp(-3.5f * t);
            sampleData[i] = (std::sin(2.0f * M_PI * 8.0f * t) + 0.4f * std::sin(2.0f * M_PI * 22.0f * t)) * env;
        }
    }
    static int markerStart = 0;
    static int markerLoopStart = 110;
    static int markerLoopEnd = 380;
    static int markerEnd = 500;

    WaveformView("##KitWaveform", sampleData.data(), sampleData.size(),
                 &markerStart, &markerLoopStart, &markerLoopEnd, &markerEnd,
                 ImVec2(-1, 80));

    ImGui::End();
}

} // namespace trakr::kit

