#include "sample_loader.h"

#include "Song.h"
#include "EngineHost.h"
#include "views/SampleLoaderView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace trakr {
using namespace ::trakr::ui;

namespace {
constexpr int kListX = 1;
constexpr int kListY = 29 + 32;                 // 61
constexpr int kListW = (800 / 8) * 3 - 3;       // 297
constexpr int kListH = 394 - 32;                // 362
constexpr int kSlotsX = (800 / 8) * 3 + 1;      // 301
constexpr int kFileLabelX = (800 / 16) * 3 - 1; // 149
constexpr int kSlotLabelX = (800 / 16) * 9 - 1; // 449
constexpr int kLabelY = 32;
constexpr int kLabelW = (800 / 8) * 3 - 3;
constexpr int kLabelH = 26;
constexpr int kPreviewX = (800 / 8) * 6 + 1;    // 601
constexpr int kMemoryX = (800 / 8) * 7 + 1;     // 701
constexpr int kLoaderBarY = 29;
constexpr int kLoaderBarH = 394;
constexpr int kLoaderBarW = 800 / 8 - 3;              // 97
constexpr int kListLines = 13;

// gSampleImport.cpp framesPlaces[3][4].
constexpr int kFramePlaces[3][4] = {
    {1, 31, kListW, 391},
    {kSlotsX, 31, kListW, 391},
    {kPreviewX, 31, kLoaderBarW, 391},
};
} // namespace

SampleLoaderScreen::SampleLoaderScreen() = default;

void SampleLoaderScreen::build() {
    if (built_) return;

    std::vector<RectI> places;
    for (const auto& p : kFramePlaces) places.push_back(RectI{p[0], p[1], p[2], p[3]});
    frame_ = &display_.add<FrameControl>("frame", std::move(places), 0);

    // gSampleImport.cpp:56-69: controlStyleCenterX, so the firmware x is the
    // label centre and the background spans [x - w/2, x + w/2].
    explorerTitle_ = &display_.add<LabelControl>("explorerTitle", "SD Card",
                                                 trakr::hw::FwFont::Bold17);
    explorerTitle_->setRect(RectI{kFileLabelX - kLabelW / 2, kLabelY, kLabelW, kLabelH});
    explorerTitle_->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1],
                              kTopBarLabelColors[2]);
    instrumentTitle_ = &display_.add<LabelControl>("instrumentTitle", "Instruments",
                                                   trakr::hw::FwFont::Bold17);
    instrumentTitle_->setRect(RectI{kSlotLabelX - kLabelW / 2, kLabelY, kLabelW, kLabelH});
    instrumentTitle_->setColors(kTopBarLabelColors[0], kTopBarLabelColors[1],
                                kTopBarLabelColors[2]);

    explorerList_ = &display_.add<ListControl>("explorerList", std::vector<std::string>{},
                                               kListLines);
    explorerList_->setRect(RectI{kListX, kListY, kListW, kListH});
    explorerList_->setColors(kSampleLoaderActiveListColors);
    instrumentList_ = &display_.add<ListControl>("instrumentList", std::vector<std::string>{},
                                                 kListLines);
    instrumentList_->setRect(RectI{kSlotsX, kListY, kListW, kListH});
    instrumentList_->setColors(kSampleLoaderInactiveListColors);

    memoryBar_ = &display_.add<BarControl>("memoryBar", 7, BarControl::Mode::Compare);
    memoryBar_->setRect(RectI{kMemoryX, kLoaderBarY, kLoaderBarW, kLoaderBarH});
    memoryBar_->setColors(kSampleLoadMemBarNormalColors);

    previewBar_ = &display_.add<BarControl>("previewVolumeBar", 6, BarControl::Mode::Fill);
    previewBar_->setRect(RectI{kPreviewX, kLoaderBarY, kLoaderBarW, kLoaderBarH});

    explorerList_->setOnSelect([this](int index) {
        if (view_) view_->selectFile(index);
    });
    instrumentList_->setOnSelect([this](int index) {
        if (view_) view_->selectSlot(index, selectedInst_);
    });

    built_ = true;
}

void SampleLoaderScreen::syncFromModel(const Song& song, const SampleLoaderView& view) {
    const SampleLoaderView::Snapshot snap = view.snapshot(song);
    explorerList_->setItems(snap.files);
    instrumentList_->setItems(snap.slots);
    explorerList_->setSelected(snap.selectedFile);
    instrumentList_->setSelected(snap.selectedSlot);
    // showFilesTree()/showInstrumentsList() set list->start to the selection.
    explorerList_->setStart(snap.selectedFile);
    instrumentList_->setStart(snap.selectedSlot);

    const ImU32* fileColors = snap.filesActive ? kSampleLoaderActiveListColors
                                               : kSampleLoaderInactiveListColors;
    const ImU32* slotColors = snap.filesActive ? kSampleLoaderInactiveListColors
                                               : kSampleLoaderActiveListColors;
    explorerList_->setColors(fileColors);
    instrumentList_->setColors(slotColors);
    explorerTitle_->setColors(fileColors[4], fileColors[3], fileColors[0]);
    instrumentTitle_->setColors(slotColors[4], slotColors[3], slotColors[0]);

    frame_->setSelected(snap.filesActive ? 0 : 1);

    if (snap.memoryFull) {
        memoryBar_->setColors(kSampleLoadMemBarFullColors);
        memoryBar_->setValue(100);
        memoryBar_->setCompareValue(100);
    } else {
        memoryBar_->setColors(kSampleLoadMemBarNormalColors);
        memoryBar_->setValue(snap.memoryUsed);
        memoryBar_->setCompareValue(snap.memoryCompare);
    }
    previewBar_->setValue(snap.previewVolume);
}

void SampleLoaderScreen::update(Song& song,
                                History& history,
                                EngineHost& engineHost,
                                int& selectedInstIdx,
                                SampleLoaderView& view,
                                const ImRect& workspace,
                                ui::LayoutProfile profile) {
    (void)history;
    build();
    song_ = &song;
    engineHost_ = &engineHost;
    view_ = &view;
    selectedInst_ = std::clamp(selectedInstIdx, 0, 47);

    ImGui::SetCursorScreenPos(workspace.Min);
    ImGui::InvisibleButton("##gui_next_loader_workspace", workspace.GetSize());
    const bool keyTarget =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || !ImGui::IsAnyItemActive();

    if (keyTarget && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) view.setFilesFocused(true);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) view.setFilesFocused(false);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (view.filesFocused()) view.moveFileSelection(-1);
            else view.moveSlotSelection(-1, selectedInstIdx);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (view.filesFocused()) view.moveFileSelection(+1);
            else view.moveSlotSelection(+1, selectedInstIdx);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && view.filesFocused())
            view.activateSelectedFile();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
        const ImVec2 device = lc.screenToDevice(ImGui::GetIO().MousePos);
        const int dx = static_cast<int>(std::floor(device.x));
        const int dy = static_cast<int>(std::floor(device.y));
        if (ui::Control* hit = display_.hitTest(dx, dy)) hit->onPress(dx, dy);
    }

    // Keep the external instrument selection in sync with the instrument pane.
    if (!view.filesFocused()) selectedInstIdx = std::clamp(selectedInst_, 0, 47);

    syncFromModel(song, view);
}

void SampleLoaderScreen::render(ImDrawList* drawList,
                                const ImRect& workspace,
                                ui::LayoutProfile profile) const {
    if (!built_ || drawList == nullptr) return;
    const LayoutContext lc = LayoutContext::fromWorkspace(workspace, profile);
    Canvas canvas(drawList, lc);
    canvas.pushClip(RectI{0, kWorkspaceTop, kDeviceWidth, kWorkspaceSpanY});
    display_.renderAll(canvas);
    canvas.popClip();
}

} // namespace trakr
