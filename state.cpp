#include "state.hpp"

void State::LoadFile(std::string file_name) {
    std::unique_ptr<AudioBuffer> ab = std::make_unique<AudioBuffer>();
    ab->LoadFile(file_name);
    if (*ab) {
        float length = ab->Length();
        Track track;
        track.path = file_name;
        track.audio_buffer = std::move(ab);
        tracks.push_back(std::move(track));
        zoom_window.LoadFile(length);
    }
}

void State::TogglePlayback() {
    if (selected_track >= 0 && selected_track < tracks.size()) {
        audio->TogglePlayback(*tracks[selected_track].audio_buffer, selection_start, selection_end);
    }
}
