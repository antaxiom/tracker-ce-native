#include <catch2/catch_test_macros.hpp>
#include "views/PatternView.h"
#include "Theme.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "AudioBackend.h"
#include "imgui.h"

using namespace trakr;

namespace {

struct ImGuiTestFixture {
    ImGuiContext* ctx{nullptr};

    ImGuiTestFixture() {
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigMacOSXBehaviors = false;
        io.DisplaySize = ImVec2(1280, 800);
        io.DeltaTime = 1.0f / 60.0f;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }

    ~ImGuiTestFixture() {
        if (ctx) ImGui::DestroyContext(ctx);
    }

    void dispatchKey(PatternView& view, Pattern& pat, int patIdx, Song& song, History& history,
                     EngineHost& host, ImGuiKey key, int octave, int editStep, bool recordMode, int instIdx,
                     bool ctrl = false, bool shift = false, bool alt = false) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(key, true);
        ImGui::NewFrame();
        io.KeyCtrl = ctrl;
        io.KeySuper = ctrl;
        io.KeyShift = shift;
        io.KeyAlt = alt;
        view.handleKeyboardInput(pat, patIdx, song, history, host, octave, editStep, recordMode, instIdx);
        ImGui::EndFrame();
        io.AddKeyEvent(key, false);
        ImGui::NewFrame();
        ImGui::EndFrame();
    }
};

} // namespace

TEST_CASE("E2E UI Note Entry and Step Advance", "[ui_e2e]") {
    ImGuiTestFixture fixture;

    Song song = makeDefaultSong();
    History history;
    auto nullAudio = std::make_unique<NullAudioBackend>();
    EngineHost engineHost(std::move(nullAudio));
    engineHost.start(44100, 128);

    PatternView patternView;
    auto& pat = song.patterns[0];

    SECTION("Piano keys enter notes and advance cursor by editStep") {
        patternView.setCursor(0, 0, 0); // track 0, row 0, subCol 0 (note)
        int octave = 4;
        int editStep = 1;
        bool recordMode = true;
        int instIdx = 0;

        // 1. Enter C-4 via 'Z'
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][0].note == 48); // C-4 = 4 * 12 + 0
        REQUIRE(pat.tracks[0][0].instrument == 0);
        REQUIRE(patternView.cursorRow() == 1); // Advanced by 1

        // 2. Enter C#4 via 'S'
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_S, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][1].note == 49); // C#4
        REQUIRE(patternView.cursorRow() == 2);

        // 3. Enter C-5 via 'Q' (upper octave)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Q, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][2].note == 60); // C-5 = 4 * 12 + 12
        REQUIRE(patternView.cursorRow() == 3);

        // 4. Enter Note-Off via 'A'
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_A, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][3].note == kNoteOff);
        REQUIRE(patternView.cursorRow() == 4);

        // 5. Enter Note-Off via '1'
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_1, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][4].note == kNoteOff);
        REQUIRE(patternView.cursorRow() == 5);
    }

    SECTION("Edit step variable advance") {
        patternView.setCursor(0, 0, 0);
        int octave = 3;
        int editStep = 4; // Advance by a full beat
        bool recordMode = true;
        int instIdx = 0;

        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][0].note == 36); // C-3
        REQUIRE(patternView.cursorRow() == 4); // Advanced 4 rows

        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_X, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[0][4].note == 38);
        REQUIRE(patternView.cursorRow() == 8);
    }

    SECTION("Hex editing on Instrument and Volume sub-columns") {
        patternView.setCursor(1, 0, 1); // Track 1, Row 0, SubCol 1 (Instrument)
        int octave = 4;
        int editStep = 0;
        bool recordMode = true;
        int instIdx = 0;

        // Enter '0' then '5' -> instrument 5 (internal 0-based idx 4)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_0, octave, editStep, recordMode, instIdx);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_5, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[1][0].instrument == 4);

        // Switch to SubCol 2 (FX1)
        patternView.setCursor(1, 0, 2);
        // Enter hex '3' then 'F' -> FX1 value 0x3F (63)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_3, octave, editStep, recordMode, instIdx);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_F, octave, editStep, recordMode, instIdx);
        REQUIRE(pat.tracks[1][0].fx[0].value == 0x3F);
    }

    SECTION("Auto-jump to note column on piano key press from non-note columns") {
        patternView.setCursor(2, 5, 1); // Cursor on Instrument sub-column
        REQUIRE(patternView.cursorSubCol() == 1);

        // User presses 'Z' (C-4 note)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, 4, 1, true, 0);

        // Sub-column auto-jumped to Note (0) and entered the note!
        REQUIRE(patternView.cursorSubCol() == 0);
        REQUIRE(pat.tracks[2][5].note == 48);
    }

    SECTION("Clear cell with Period and Delete") {
        pat.tracks[0][10].note = 50;
        pat.tracks[0][10].instrument = 2;
        pat.tracks[0][10].volume = 40;

        patternView.setCursor(0, 10, 0);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Period, 4, 0, true, 0);
        REQUIRE(pat.tracks[0][10].note == kNoNote);
        REQUIRE(pat.tracks[0][10].instrument == -1);

        pat.tracks[0][12].note = 55;
        patternView.setCursor(0, 12, 0);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Delete, 4, 0, true, 0);
        REQUIRE(pat.tracks[0][12].note == kNoNote);
    }

    SECTION("Audition vs Record Mode Invariant (tracker-manual-1.7.0.txt sec 5.3)") {
        patternView.setCursor(0, 7, 0);
        bool recordModeFalse = false;

        // Transport is stopped (!isPlaying)
        REQUIRE_FALSE(engineHost.getPlayhead().playing);

        // Press 'Z' in Green (audition) mode: step is not modified
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, 4, 1, recordModeFalse, 0);
        REQUIRE(pat.tracks[0][7].note == kNoNote);

        // Press 'Z' in Red (record) mode: step is recorded
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, 4, 1, true, 0);
        REQUIRE(pat.tracks[0][7].note == 48);
    }

    SECTION("Undo and Redo restores and reapplies note edits") {
        patternView.setCursor(3, 15, 0);

        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_Z, 4, 1, true, 0);
        REQUIRE(pat.tracks[3][15].note == 48);
        REQUIRE(history.canUndo());

        // Undo
        history.undo(song);
        REQUIRE(pat.tracks[3][15].note == kNoNote);
        REQUIRE(history.canRedo());

        // Redo
        history.redo(song);
        REQUIRE(pat.tracks[3][15].note == 48);
    }

    SECTION("Ctrl+Arrow semitone and octave transposition") {
        pat.tracks[0][20].note = 48; // C-4
        patternView.setCursor(0, 20, 0);

        // Transpose up 1 semitone (Ctrl + UpArrow)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_UpArrow, 4, 0, true, 0, true, false, false);
        REQUIRE(pat.tracks[0][20].note == 49); // C#4

        // Transpose up 1 octave (Ctrl + Shift + UpArrow)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_UpArrow, 4, 0, true, 0, true, true, false);
        REQUIRE(pat.tracks[0][20].note == 61); // C#5
    }

    SECTION("Clipboard Copy and Paste") {
        pat.tracks[0][30].note = 52; // E-4
        pat.tracks[0][30].instrument = 3;
        pat.tracks[0][30].volume = 38;

        patternView.setCursor(0, 30, 0);

        // Copy (Ctrl + C)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_C, 4, 0, true, 0, true, false, false);

        // Move to row 32
        patternView.setCursor(0, 32, 0);

        // Paste (Ctrl + V)
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_V, 4, 0, true, 0, true, false, false);

        REQUIRE(pat.tracks[0][32].note == 52);
        REQUIRE(pat.tracks[0][32].instrument == 3);
        REQUIRE(pat.tracks[0][32].volume == 38);
    }

    SECTION("FX1 and FX2 Parameter Entry and PatternView Theme Rendering") {
        patternView.setCursor(0, 40, 2); // FX1 sub-column
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_7, 4, 0, true, 0);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_F, 4, 0, true, 0);
        REQUIRE(pat.tracks[0][40].fx[0].value == 0x7F);

        patternView.setCursor(0, 40, 3); // FX2 sub-column
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_2, 4, 0, true, 0);
        fixture.dispatchKey(patternView, pat, 0, song, history, engineHost, ImGuiKey_0, 4, 0, true, 0);
        REQUIRE(pat.tracks[0][40].fx[1].value == 0x20);

        // Verify update runs cleanly under Polyend and High-Contrast themes
        Theme polyendTheme = makePolyendHardwareTheme();
        Theme hiContrast = makeColorblindTheme();
        ImGui::NewFrame();
        patternView.update(song, history, engineHost, 0, 4, 1, false, 0, &polyendTheme);
        patternView.update(song, history, engineHost, 0, 4, 1, true, 0, &hiContrast);
        ImGui::EndFrame();
    }

    SECTION("Selection Alteration: Relative and Uniform lines of notes per instrument") {
        // Set up 4 lines on Track 0 with a C-major chord arpeggio
        pat.tracks[0][0] = Cell{48, 1, 20}; // C-4
        pat.tracks[0][1] = Cell{52, 1, 30}; // E-4
        pat.tracks[0][2] = Cell{55, 1, 40}; // G-4
        pat.tracks[0][3] = Cell{60, 1, 50}; // C-5

        // Select lines 0..3 on Track 0
        patternView.setSelection(0, 0, 0, 3);
        REQUIRE(patternView.hasSelection());
        auto sel = patternView.getSelection();
        REQUIRE(sel.rowCount() == 4);
        REQUIRE(sel.trackCount() == 1);

        // 1. Relative Transpose (+2 semitones)
        transposeSelection(pat, 0, sel, 2, history, song, engineHost);
        REQUIRE(pat.tracks[0][0].note == 50); // D-4
        REQUIRE(pat.tracks[0][1].note == 54); // F#4
        REQUIRE(pat.tracks[0][2].note == 57); // A-4
        REQUIRE(pat.tracks[0][3].note == 62); // D-5

        // Undo relative transpose
        history.undo(song);
        REQUIRE(pat.tracks[0][0].note == 48);
        REQUIRE(pat.tracks[0][1].note == 52);

        // Redo relative transpose
        history.redo(song);
        REQUIRE(pat.tracks[0][0].note == 50);
        REQUIRE(pat.tracks[0][1].note == 54);

        // Relative Octave Transpose (-12 semitones)
        transposeSelection(pat, 0, sel, -12, history, song, engineHost);
        REQUIRE(pat.tracks[0][0].note == 38); // D-3
        REQUIRE(pat.tracks[0][1].note == 42); // F#3
        REQUIRE(pat.tracks[0][2].note == 45); // A-3
        REQUIRE(pat.tracks[0][3].note == 50); // D-4

        // 2. Relative Volume Nudge (+4)
        adjustVolumeSelection(pat, 0, sel, 4, history, song, engineHost);
        REQUIRE(pat.tracks[0][0].volume == 24);
        REQUIRE(pat.tracks[0][1].volume == 34);
        REQUIRE(pat.tracks[0][2].volume == 44);
        REQUIRE(pat.tracks[0][3].volume == 54);

        // 3. Uniform Alteration: Set all lines to the SAME note and instrument
        setSelectionToNote(pat, 0, sel, 64, 3, history, song, engineHost); // E-5, Inst 3
        for (int r = 0; r < 4; ++r) {
            REQUIRE(pat.tracks[0][r].note == 64);
            REQUIRE(pat.tracks[0][r].instrument == 3);
        }

        // 4. Uniform Instrument: Set all lines to Instrument 7
        setSelectionInstrument(pat, 0, sel, 7, history, song, engineHost);
        for (int r = 0; r < 4; ++r) {
            REQUIRE(pat.tracks[0][r].instrument == 7);
        }

        // 5. Uniform Volume: Set all lines to Volume 40
        setSelectionVolume(pat, 0, sel, 40, history, song, engineHost);
        for (int r = 0; r < 4; ++r) {
            REQUIRE(pat.tracks[0][r].volume == 40);
        }

        // 6. Clear Selection
        clearSelection(pat, 0, sel, -1, history, song, engineHost);
        for (int r = 0; r < 4; ++r) {
            REQUIRE(pat.tracks[0][r].note == kNoNote);
            REQUIRE(pat.tracks[0][r].instrument == -1);
        }

        // Deselect
        patternView.clearSelection();
        REQUIRE(!patternView.hasSelection());
    }

    engineHost.stop();
}
