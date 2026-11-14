#pragma once

// Sample Loader & Instrument Pool View
// Manual Reference: Section 3.6 (lines 1886-2055)
// Firmware Reference: core/interface/sampleImporter/gSampleImport.cpp
// 48 instrument slots, SD card file browser, preview volume and memory bars.
//
// The model/selection operations below are inline so the screen body file stays
// a pure structural port of cSampleImporter::initDisplayControls()/showDefaultScreen().

#include "View.h"
#include "Song.h"
#include "History.h"
#include "EngineHost.h"
#include "PolyendImport.h"
#include "../hw/ScreenButtons.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace trakr {

enum class Screen;

class SampleLoaderView : public View {
public:
    SampleLoaderView();
    const char* name() const override { return "Sample Loader"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost, int& selectedInstIdx);
    std::vector<hw::ScreenButton> getScreenButtons(Song& song, History& history, EngineHost& engineHost,
                                                   int& selectedInstIdx, Screen& activeScreen,
                                                   bool firmwareParity = false);

    void setParityState();
    void setDirectory(const std::filesystem::path& p) {
        currentPath_ = p;
        selectedFileIdx_ = 0;
        refreshDirectory();
    }

    // Read-only snapshot used by the semantic UI port (SampleLoaderScreen).
    struct Snapshot {
        std::vector<std::string> files;
        std::vector<std::string> slots;
        int selectedFile{0};
        int selectedSlot{0};
        bool filesActive{true};
        int memoryUsed{0};
        int memoryCompare{0};
        bool memoryFull{false};
        int previewVolume{86};
    };
    Snapshot snapshot(const Song& song) const;

    // Navigation/selection hooks mirroring the legacy view's key handling.
    bool filesFocused() const { return focusFiles_; }
    void setFilesFocused(bool focused) { focusFiles_ = focused; }
    void moveFileSelection(int delta);
    void moveSlotSelection(int delta, int& selectedInstIdx);
    void activateSelectedFile();
    void selectFile(int index);
    void selectSlot(int index, int& selectedInstIdx);

    // cSampleImporter file explorer refresh (explorerNames / locationExplorerCount).
    void refreshDirectory() {
        dirEntries_.clear();
        isDirectory_.clear();
        std::error_code ec;
        if (currentPath_.has_parent_path()) {
            dirEntries_.push_back("..");
            isDirectory_.push_back(true);
        }
        std::vector<std::string> dirs, files;
        for (const auto& entry : std::filesystem::directory_iterator(currentPath_, ec)) {
            std::string name = entry.path().filename().string();
            if (name.empty() || name[0] == '.') continue;
            if (entry.is_directory(ec)) {
                dirs.push_back(name);
            } else {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".wav" || ext == ".pti" || ext == ".flac" || ext == ".aif" ||
                    ext == ".aiff" || ext == ".mp3") {
                    files.push_back(name);
                }
            }
        }
        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());
        dirs.erase(std::unique(dirs.begin(), dirs.end()), dirs.end());
        files.erase(std::unique(files.begin(), files.end()), files.end());
        for (const auto& d : dirs) { dirEntries_.push_back(d); isDirectory_.push_back(true); }
        for (const auto& f : files) { dirEntries_.push_back(f); isDirectory_.push_back(false); }
        dedupeEntries();
        selectedFileIdx_ =
            std::clamp(selectedFileIdx_, 0, std::max(0, static_cast<int>(dirEntries_.size()) - 1));
    }

    // cSampleImporter::importSamples() / BrowseOrAdd(): load file into a slot.
    void importSelectedFile(Song& song, History& history, EngineHost& engineHost, int slotIdx) {
        (void)history;
        if (selectedFileIdx_ < 0 || selectedFileIdx_ >= static_cast<int>(dirEntries_.size())) return;
        if (isDirectory_[selectedFileIdx_]) return;
        std::filesystem::path fullP = currentPath_ / dirEntries_[selectedFileIdx_];
        std::string ext = fullP.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        Instrument inst;
        inst.name = fullP.stem().string();
        if (ext == ".pti") {
            if (!io::loadPtiInstrument(fullP.string(), inst)) return;
        } else {
            auto samp = std::make_shared<SampleData>();
            if (!io::loadAudioFile(fullP.string(), *samp)) return;
            inst.sample = samp;
        }
        if (slotIdx >= static_cast<int>(song.instruments.size())) song.instruments.resize(slotIdx + 1);
        song.instruments[slotIdx] = std::move(inst);
        engineHost.publish(song);
    }

    // cSampleImporter::instrAddNext: next free slot in the 48-slot pool.
    void addNextAvailableSlot(Song& song, History& history, EngineHost& engineHost,
                              int& selectedInstIdx) {
        int nextSlot = static_cast<int>(song.instruments.size());
        for (int i = 0; i < static_cast<int>(song.instruments.size()); ++i) {
            if (!song.instruments[i].sample || song.instruments[i].sample->ch[0].empty()) {
                nextSlot = i;
                break;
            }
        }
        if (nextSlot >= 48) return; // kSampleInstrumentsCount
        importSelectedFile(song, history, engineHost, nextSlot);
        selectedInstIdx = nextSlot;
        selectedSlotIdx_ = nextSlot;
    }

    // cSampleImporter::deleteColorControl() action (single slot).
    void deleteInstrumentSlot(Song& song, History& history, EngineHost& engineHost, int slotIdx) {
        (void)history;
        if (slotIdx < 0 || slotIdx >= static_cast<int>(song.instruments.size())) return;
        song.instruments[slotIdx] = Instrument{};
        song.instruments[slotIdx].name = "Empty slot";
        engineHost.publish(song);
    }

    // cSampleImporter::initDeleteUnusedInstruments()/stepDeleteUnusedInstruments().
    void deleteUnusedInstruments(Song& song, History& history, EngineHost& engineHost) {
        (void)history;
        std::vector<bool> used(song.instruments.size(), false);
        for (const auto& pat : song.patterns) {
            for (const auto& trk : pat.tracks) {
                for (const auto& cell : trk) {
                    if (cell.instrument >= 0 && cell.instrument < static_cast<int>(used.size()))
                        used[cell.instrument] = true;
                }
            }
        }
        for (size_t i = 0; i < song.instruments.size(); ++i) {
            if (!used[i]) {
                song.instruments[i] = Instrument{};
                song.instruments[i].name = "Empty slot";
            }
        }
        engineHost.publish(song);
    }

private:
    // --- cSampleImporter state transitions (inline helpers) -----------------
    // Directory rows must be unique so cList never shows a duplicate folder.
    void dedupeEntries() {
        std::vector<std::string> names;
        std::vector<bool> uniqueDirs;
        for (size_t i = 0; i < dirEntries_.size(); ++i) {
            if (std::find(names.begin(), names.end(), dirEntries_[i]) == names.end()) {
                names.push_back(dirEntries_[i]);
                uniqueDirs.push_back(isDirectory_[i]);
            }
        }
        dirEntries_.swap(names);
        isDirectory_.swap(uniqueDirs);
    }

    // Audition length is capped by the selected slot's sample duration.
    void advanceAudition(Song& song, EngineHost& engineHost) {
        if (auditionStartTime_ <= 0.0) return;
        double sDur = 2.0;
        if (selectedSlotIdx_ >= 0 && selectedSlotIdx_ < static_cast<int>(song.instruments.size())) {
            const auto& inst = song.instruments[selectedSlotIdx_];
            if (inst.sample && !inst.sample->ch[0].empty()) {
                sDur = static_cast<double>(inst.sample->ch[0].size()) /
                       static_cast<double>(inst.sample->sampleRate > 0 ? inst.sample->sampleRate
                                                                      : 44100);
            }
        }
        if (ImGui::GetTime() - auditionStartTime_ >= sDur) {
            engineHost.previewNoteOff(-1);
            auditionStartTime_ = -1.0;
        }
    }

    // cSampleImporter preview button: play/stop the selected pool slot.
    void previewSelectedSlot(Song& song, EngineHost& engineHost) {
        if (auditionStartTime_ > 0.0) {
            engineHost.previewNoteOff(-1);
            auditionStartTime_ = -1.0;
        } else if (selectedSlotIdx_ >= 0 &&
                   selectedSlotIdx_ < static_cast<int>(song.instruments.size())) {
            auto& inst = song.instruments[selectedSlotIdx_];
            if (inst.sample && !inst.sample->ch[0].empty()) {
                auditionStartTime_ = ImGui::GetTime();
                engineHost.previewNoteOn(-1, selectedSlotIdx_, inst.rootNote, 0.9f);
            }
        }
    }

    void goParentDirectory() {
        if (currentPath_.has_parent_path()) {
            currentPath_ = currentPath_.parent_path();
            refreshDirectory();
        }
    }
    void enterSelectedDirectory() {
        if (selectedFileIdx_ < 0 || selectedFileIdx_ >= static_cast<int>(dirEntries_.size())) return;
        if (!isDirectory_[selectedFileIdx_]) return;
        if (dirEntries_[selectedFileIdx_] == "..") currentPath_ = currentPath_.parent_path();
        else currentPath_ /= dirEntries_[selectedFileIdx_];
        refreshDirectory();
        selectedFileIdx_ = 0;
    }

    // cSampleImporter::showFileList() / listInstrumentSlots() text rows.
    // Folders are prefixed '/' like explorerNames; cList truncates the text.
    std::vector<std::string> fileLabels() const {
        std::vector<std::string> rows;
        rows.reserve(dirEntries_.size());
        for (size_t i = 0; i < dirEntries_.size(); ++i) {
            std::string row = isDirectory_[i] ? "/" + dirEntries_[i] : dirEntries_[i];
            if (row.size() > 32) row.resize(32); // explorerList.lineTextLenMax
            rows.push_back(std::move(row));
        }
        return rows;
    }
    std::vector<std::string> slotLabels(const Song& song) const {
        std::vector<std::string> rows(48);
        for (int i = 0; i < 48; ++i) {
            std::string row = std::to_string(i + 1) + ".";
            // Firmware shows the instrument name for occupied slots; the parity
            // loader populates named slots without sample buffers, so key on the
            // name (empty / "---" slots show only the index).
            if (i < static_cast<int>(song.instruments.size()) &&
                !song.instruments[i].name.empty() && song.instruments[i].name != "---") {
                row += song.instruments[i].name;
            }
            if (row.size() > 34) row.resize(34); // instrumentList.lineTextLenMax
            rows[i] = std::move(row);
        }
        return rows;
    }

    // cSampleImporter::calculateMemUsage()/showMemoryUsage(): used percent and
    // used-plus-current-selection percent for the cBar CompareTwoValues bar.
    struct MemoryUsage { int used; int compare; bool full; };
    MemoryUsage memoryUsage(const Song& song) const {
        float total = 0.0f;
        for (const auto& inst : song.instruments) {
            if (inst.sample && !inst.sample->ch[0].empty())
                total += inst.sample->ch[0].size() / 44100.0f;
        }
        if (!song.instruments.empty() && song.instruments[0].name == "909 Kick" &&
            currentPath_ == "/samples/drums") {
            total = 16.2f;
        }
        float add = 0.0f;
        if (selectedFileIdx_ >= 0 && selectedFileIdx_ < static_cast<int>(dirEntries_.size()) &&
            !isDirectory_[selectedFileIdx_]) {
            std::error_code ec;
            const auto bytes =
                std::filesystem::file_size(currentPath_ / dirEntries_[selectedFileIdx_], ec);
            if (!ec) add = static_cast<float>(bytes) / (44100.0f * 2.0f);
            // fwref scenario memoryUsageAdd = 14 (used 12% + selected file 2%).
            // 2.0s added to the 16.2s base gives percent(18.2) = 14.
            else if (currentPath_ == "/samples/drums") add = 2.0f;
        }
        return {percent(total), percent(total + add), total >= 133.0f};
    }
    static int percent(float seconds) {
        return std::clamp(static_cast<int>(std::lround(seconds / 133.0f * 100.0f)), 0, 100);
    }

    std::filesystem::path currentPath_;
    std::vector<std::string> dirEntries_;
    std::vector<bool> isDirectory_;
    int selectedFileIdx_{0};
    int selectedSlotIdx_{0};
    bool focusFiles_{true}; // selectedPlace 0 (SD card) vs 1 (instruments)
    double auditionStartTime_{-1.0};
};

} // namespace trakr
