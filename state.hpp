#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>
#include "audio_buffer.hpp"
#include "audio_system.hpp"
#include "gl_waveform.hpp"
#include "zoom_window.hpp"

struct Track {
    std::string path;
    std::unique_ptr<AudioBuffer> audio_buffer;
    std::unique_ptr<GLWaveform> gpu_buffer;
};

class State {
   public:
    State(AudioSystem* audio) : audio(audio){};
    void LoadFile(std::string file_name);
    void TogglePlayback();
    void UpdateGpuBuffers();
    std::vector<Track> tracks;
    ZoomWindow zoom_window;
    float selection_start = 0.f;
    float selection_end = -1.f;
    int selected_track = 0;

   private:
    AudioSystem* audio;
    int next_id = 0;
};

#endif
