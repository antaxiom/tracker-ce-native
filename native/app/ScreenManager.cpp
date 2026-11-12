#include "ScreenManager.h"

namespace trakr {

void renderActiveScreen(Screen activeScreen, ScreenContext& ctx, const ImRect& workspaceRect) {
    ImGui::SetNextWindowPos(workspaceRect.Min);
    ImGui::SetNextWindowSize(workspaceRect.GetSize());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    switch (activeScreen) {
        case Screen::Pattern:
            ctx.patternView.update(ctx.song, ctx.history, ctx.engineHost, ctx.currentOrderIdx,
                                   ctx.octave, ctx.editStep, ctx.recordMode, ctx.selectedInstIdx, &ctx.theme);
            break;
        case Screen::Song:
            ctx.orderListView.update(ctx.song, ctx.history, ctx.engineHost, ctx.currentOrderIdx);
            break;
        case Screen::Sample:
            ctx.sampleView.update(ctx.song, ctx.history, ctx.engineHost, ctx.selectedInstIdx);
            break;
        case Screen::Instrument:
            ctx.instrumentView.update(ctx.song, ctx.history, ctx.engineHost, ctx.selectedInstIdx);
            break;
        case Screen::Master:
            ctx.mixerView.update(ctx.song, ctx.history, ctx.engineHost);
            break;
        case Screen::Perform:
            ctx.performView.update(ctx.song, ctx.history, ctx.engineHost);
            break;
        case Screen::SampleLoader:
            ctx.sampleLoaderView.update(ctx.song, ctx.history, ctx.engineHost, ctx.selectedInstIdx);
            break;
        case Screen::Config:
            ctx.configView.update(ctx.song, ctx.engineHost, ctx.theme);
            break;
        case Screen::File:
            ctx.fileView.update(ctx.song, ctx.engineHost);
            break;
    }

    ImGui::PopStyleVar(3);
}

void renderActiveScreen(Screen activeScreen, ScreenContext& ctx, float headerHeight, float footerHeight) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    ImVec2 screenPos(vp->WorkPos.x, vp->WorkPos.y + headerHeight);
    ImVec2 screenSize(vp->WorkSize.x, vp->WorkSize.y - headerHeight - footerHeight);
    renderActiveScreen(activeScreen, ctx, ImRect(screenPos, ImVec2(screenPos.x + screenSize.x, screenPos.y + screenSize.y)));
}

} // namespace trakr
