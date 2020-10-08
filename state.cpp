#include "state.hpp"

void State::LoadFile(std::string file_name) {
    std::shared_ptr<AudioBuffer> ab = std::make_shared<AudioBuffer>();
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
    if (SelectedTrack()) {
        audio->TogglePlayback(tracks[*selected_track].audio_buffer, Cursor(), Selection());
        last_played_track = *selected_track;
    }
}

void State::UpdateGpuBuffers() {
    for (Track& t : tracks) {
        if (!t.gpu_buffer) {
            t.gpu_buffer = std::make_unique<GLWaveform>(*t.audio_buffer);
        }
    }
}

void State::DeleteGpuBuffers() {
    for (Track& t : tracks) {
        if (!t.gpu_buffer) {
            t.gpu_buffer.reset();
        }
    }
}

bool State::Playing(float* time) {
    return audio->Playing(time);
}

bool State::SetSelectedTrack(int track) {
    if (track < 0 || track >= static_cast<int>(tracks.size()))
        return false;
    else if (*selected_track && *selected_track == track)
        return false;
    else {
        selected_track = track;
        return true;
    }
}
