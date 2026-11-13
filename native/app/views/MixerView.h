#pragma once

// Master screen structural port.
// Firmware reference: core/interface/masterParams/gMasterParams.cpp, masterParams.h
// Manual reference: Section 8.1 Master Pages - Audio Control (lines 10561-10660)

#include "View.h"
#include "Song.h"
#include "FaderCurve.h"
#include "History.h"
#include "EngineHost.h"

namespace trakr {

class MixerView : public View {
public:
    // Master 1/3, Track Mixer 2/3, Global Mixer 3/3 and the Master EQ page
    // (reachable from the EQ label on Master 1/3, like gMasterParams).
    enum class SubPage { Master = 0, TrackMixer = 1, GlobalMixer = 2, MasterFx = 3 };

    MixerView();
    const char* name() const override { return "Master / Mixer"; }
    void draw(bool* p_open = nullptr) override;

    void update(Song& song, History& history, EngineHost& engineHost);

    SubPage subPage() const { return subPage_; }
    void setSubPage(SubPage sp) { subPage_ = sp; }
    void togglePage() {
        subPage_ = (subPage_ == SubPage::Master) ? SubPage::TrackMixer
                 : (subPage_ == SubPage::TrackMixer) ? SubPage::GlobalMixer
                 : SubPage::Master;
    }
    // Firmware gain fader mapping used by the Track Mixer page
    // (mixAmpLogValues: 0 = silence, 86 = 0 dB, 100 = +6 dB).
    static int gainBarValue(float gainDb) { return faderValueForGainDb(gainDb); }
    int selectedTrack() const { return selectedTrack_; }
    void setSelectedTrack(int t) { selectedTrack_ = t; }
    void selectNextTrack(int delta) { selectedTrack_ = std::clamp(selectedTrack_ + delta, 0, 7); }

    void adjustSelectedParam(Song& song, EngineHost& engineHost, int delta, bool fine) {
        if (subPage_ == SubPage::TrackMixer && selectedTrack_ >= 0 && selectedTrack_ < static_cast<int>(song.tracks.size())) {
            float step = fine ? 0.1f : 1.0f;
            song.tracks[selectedTrack_].gainDb =
                std::clamp(song.tracks[selectedTrack_].gainDb + delta * step, -36.0f, 6.0f);
            engineHost.publish(song);
        } else if (subPage_ == SubPage::Master) {
            float step = fine ? 0.1f : 1.0f;
            switch (selectedTrack_) {
                case 0: song.masterGainDb = std::clamp(song.masterGainDb + delta * step, -48.0f, 6.0f); break;
                case 5: song.masterSaturationType = std::clamp(song.masterSaturationType + (delta > 0 ? 1 : -1), 0, 4); break;
                case 6: song.masterBassBoost = std::clamp(song.masterBassBoost + delta * (fine ? 0.01f : 0.05f), 0.0f, 1.0f); break;
                case 7: song.masterSpace = std::clamp(song.masterSpace + delta * (fine ? 0.01f : 0.125f), 0.0f, 1.0f); break;
                default: break;
            }
            engineHost.publish(song);
        } else if (subPage_ == SubPage::GlobalMixer) {
            // gMasterParams.cpp showGlobalMixer(): aux faders are 0..100.
            const int aux = std::clamp(selectedTrack_, 0, 3);
            song.auxVolume[aux] = std::clamp(song.auxVolume[aux] + delta, 0, 100);
            engineHost.publish(song);
        } else if (subPage_ == SubPage::MasterFx) {
            int idx = std::clamp(selectedTrack_, 0, 4);
            song.masterEq[idx] = std::clamp(song.masterEq[idx] + delta * (fine ? 0.2f : 1.0f), -12.0f, 12.0f);
            engineHost.publish(song);
        }
    }

    // Aux (Delay/Reverb/Dry Mix/Line In) helpers used by the Global Mixer page.
    static const char* auxName(int aux) {
        static const char* const kNames[4] = {"Delay", "Reverb", "Dry Mix", "Line In"};
        return (aux >= 0 && aux < 4) ? kNames[aux] : "";
    }
    static float auxGainDb(int volume) {
        const float db = gainDbForFaderValue(std::clamp(volume, 0, 100));
        return db <= kFaderSilenceDb ? -99.0f : db;
    }
    void toggleAuxMute(Song& song, EngineHost& engineHost, int aux) {
        if (aux < 0 || aux >= 4) return;
        song.auxMute[aux] = !song.auxMute[aux];
        engineHost.publish(song);
    }

    void resetSelected(Song& song, EngineHost& engineHost) {
        if (subPage_ == SubPage::TrackMixer && selectedTrack_ >= 0 && selectedTrack_ < static_cast<int>(song.tracks.size())) {
            song.tracks[selectedTrack_].gainDb = 0.0f;
            song.tracks[selectedTrack_].pan = 0.0f;
            engineHost.publish(song);
        } else if (subPage_ == SubPage::Master) {
            switch (selectedTrack_) {
                case 0: song.masterGainDb = 0.0f; break;
                case 5: song.masterSaturationType = 0; break;
                case 6: song.masterBassBoost = 0.0f; break;
                case 7: song.masterSpace = 0.0f; break;
                default: break;
            }
            engineHost.publish(song);
        } else if (subPage_ == SubPage::MasterFx) {
            int idx = std::clamp(selectedTrack_, 0, 4);
            song.masterEq[idx] = 0.0f;
            engineHost.publish(song);
        }
    }

private:
    SubPage subPage_{SubPage::Master};
    int selectedTrack_{0};
    int renamingTrack_{-1};
    char renameBuf_[32]{0};
};

} // namespace trakr
