#include "state.hpp"

void State::LoadFile(std::string file_name) {
    AudioBuffer* ab = new AudioBuffer();
    ab->LoadFile(file_name);
    if (*ab) {
        buffers[next_id++] = ab;
        zoom_window.LoadFile(static_cast<float>(ab->NumFrames()) / ab->Samplerate());
    }
}

void State::TogglePlayback() {
    if (buffers.size()) {
        auto it = buffers.begin();
        for (int i = 0; i < selected_track; i++) {
            ++it;
            if (it == buffers.end())
                return;
        }
        const AudioBuffer* ab = it->second;
        audio->TogglePlayback(ab, selection_start, selection_end);
    }
}
