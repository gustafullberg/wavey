#ifndef STATE_HPP
#define STATE_HPP

#include <future>
#include <list>
#include <memory>
#include <optional>

#include "audio_buffer.hpp"
#include "audio_system.hpp"
#include "file_load_server.hpp"
#include "file_notification.hpp"
#include "gpu_spectrogram.hpp"
#include "gpu_waveform.hpp"
#include "low_res_waveform.hpp"
#include "spectrogram.hpp"
#include "zoom_window.hpp"

struct Track {
    Track(const std::string& filename, std::optional<std::string> track_label = std::nullopt);
    std::string path;
    std::string short_name;
    std::string status;
    uint64_t mod_time;
    std::shared_ptr<AudioBuffer> audio_buffer;
    std::unique_ptr<Spectrogram> spectrogram;
    std::unique_ptr<GpuWaveform> gpu_waveform;
    std::unique_ptr<GpuSpectrogram> gpu_spectrogram;
    std::future<std::shared_ptr<AudioBuffer>> future_audio_buffer;
    std::future<std::unique_ptr<LowResWaveform>> future_lowres_waveform;
    std::future<std::unique_ptr<Spectrogram>> future_spectrogram;
    bool reload = false;
    bool remove = false;
    std::optional<int> watch_id_;
    int GetSamplerate() const { return audio_buffer ? audio_buffer->Samplerate() : 0; }
    void Reload();
    std::optional<int> selected_channel;
};

enum ViewMode { ALL, TRACK };

class State {
   public:
    State(AudioSystem* audio)
        : audio(audio),
          file_load_server([this](const std::string& file_name) { this->LoadFile(file_name); }) {}
    void LoadFile(const std::string& file_name, std::optional<std::string> label = std::nullopt);
    void UnloadFiles();
    void UnloadSelectedTrack();
    void ReloadFiles();
    bool CreateResources();
    void SetLooping(bool do_loop);
    void TogglePlayback();
    bool Playing(float* time);
    float Cursor() { return cursor; }
    void SetCursor(float time) {
        cursor = time;
        selection.reset();
    }
    void FixSelection() {
        if (Selection()) {
            if (*selection < cursor) {
                std::swap(cursor, *selection);
            }
        }
    }
    std::optional<float> Selection() { return selection; }
    void SetSelection(float time) { selection = time; }
    std::optional<int> SelectedTrack() { return selected_track; }
    bool SetSelectedTrack(int track);
    Track& GetTrack(int number);
    Track& GetSelectedTrack();
    void ResetView();
    int GetCurrentSamplerate();
    void ToggleViewSingleTrack();
    void ToggleViewSingleChannel(float mouse_y);
    ViewMode GetViewMode() { return view_mode; }
    void ScrollTrackUp();
    void ScrollTrackDown();
    void ScrollChannelUp();
    void ScrollChannelDown();

    void MoveTrackUp();
    void MoveTrackDown();

    bool DoAutoRefresh() const { return track_change_notifier_.has_value(); }
    void StartMonitoringTrackChange();
    void StopMonitoringTrackChange();

    std::list<Track> tracks;
    ZoomWindow zoom_window;
    ViewMode view_mode = ALL;
    int last_played_track = 0;

   private:
    void LoadListOfFiles(const std::string& file_name);

    void MonitorTrack(Track& t);
    void UnmonitorTrack(Track& t);

    AudioSystem* audio;
    int next_id = 0;
    float cursor = 0.f;
    std::optional<float> selection;
    std::optional<int> selected_track;

    std::optional<FileModificationNotifier> track_change_notifier_;
    FileLoadServer file_load_server;
};

#endif
