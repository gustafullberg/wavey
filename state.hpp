#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>
#include "audio_buffer.hpp"
#include "audio_system.hpp"
#include "gpu_spectrogram.hpp"
#include "gpu_waveform.hpp"
#include "spectrogram.hpp"
#include "zoom_window.hpp"

struct Track {
    std::string path;
    std::shared_ptr<AudioBuffer> audio_buffer;
    std::unique_ptr<Spectrogram> spectrogram;
    std::unique_ptr<GpuWaveform> gpu_waveform;
    std::unique_ptr<GpuSpectrogram> gpu_spectrogram;
};

class State {
   public:
    State(AudioSystem* audio) : audio(audio){};
    void LoadFile(std::string file_name);
    void TogglePlayback();
    void UpdateGpuBuffers();
    void DeleteGpuBuffers();
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
    std::optional<float> SelectedTrack() { return selected_track; }
    bool SetSelectedTrack(int track);

    std::vector<Track> tracks;
    ZoomWindow zoom_window;
    int last_played_track = 0;

   private:
    AudioSystem* audio;
    int next_id = 0;
    float cursor = 0.f;
    std::optional<float> selection;
    std::optional<int> selected_track;
};

#endif
