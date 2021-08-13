#ifndef STATE_HPP
#define STATE_HPP

#include <chrono>
#include <future>
#include <list>
#include <map>
#include <memory>
#include "audio_buffer.hpp"
#include "audio_system.hpp"
#include "gpu_label.hpp"
#include "gpu_spectrogram.hpp"
#include "gpu_waveform.hpp"
#include "label.hpp"
#include "spectrogram.hpp"
#include "zoom_window.hpp"

struct Track {
    std::string path;
    std::string short_name;
    uint64_t mod_time;
    std::shared_ptr<AudioBuffer> audio_buffer;
    std::unique_ptr<Spectrogram> spectrogram;
    std::unique_ptr<Label> label;
    std::vector<std::unique_ptr<Label>> channel_labels;
    std::unique_ptr<GpuWaveform> gpu_waveform;
    std::unique_ptr<GpuSpectrogram> gpu_spectrogram;
    std::unique_ptr<GpuLabel> gpu_track_label;
    std::vector<std::unique_ptr<GpuLabel>> gpu_channel_labels;
    std::future<std::shared_ptr<AudioBuffer>> future_audio_buffer;
    std::future<std::unique_ptr<Spectrogram>> future_spectrogram;
    bool reload = false;
    bool remove = false;

    int GetSamplerate() { return audio_buffer ? audio_buffer->Samplerate() : 0; }
};

enum ViewMode { ALL, TRACK, CHANNEL };

class State {
   public:
    State(AudioSystem* audio) : audio(audio){};
    void LoadFile(const std::string& file_name);
    void UnloadFiles();
    void UnloadSelectedTrack();
    void ReloadFiles();
    bool CreateResources(bool* view_reset);
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
    void ScrollUp();
    void ScrollDown();
    void MoveTrackUp();
    void MoveTrackDown();
    bool HasTimeLabel(const std::string& time);
    const GpuLabel& GetTimeLabel(const std::string& time) const;

    std::list<Track> tracks;
    std::map<std::string, std::unique_ptr<Label>> time_labels;
    std::map<std::string, std::unique_ptr<GpuLabel>> gpu_time_labels;
    ZoomWindow zoom_window;
    ViewMode view_mode = ALL;
    int last_played_track = 0;
    int max_timelabel_height = 0;

   private:
    void LoadListOfFiles(const std::string& file_name);
    AudioSystem* audio;
    int next_id = 0;
    float cursor = 0.f;
    std::optional<float> selection;
    std::optional<int> selected_track;
    std::optional<int> selected_channel;
};

#endif
