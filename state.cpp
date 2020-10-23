#include "state.hpp"

void State::LoadFile(std::string file_name) {
    std::shared_ptr<AudioBuffer> ab = std::make_shared<AudioBuffer>();
    ab->LoadFile(file_name);
    if (*ab) {
        float length = ab->Length();
        Track track;
        track.path = file_name;

        const size_t separator_pos = file_name.rfind('/');
        track.short_name =
            separator_pos != std::string::npos ? file_name.substr(separator_pos + 1) : file_name;
        track.audio_buffer = std::move(ab);
        track.spectrogram = std::make_unique<Spectrogram>(track.audio_buffer->Samples(),
                                                          track.audio_buffer->NumChannels(),
                                                          track.audio_buffer->NumFrames());
        track.gpu_waveform = std::make_unique<GpuWaveform>(*track.audio_buffer);
        track.gpu_spectrogram =
            std::make_unique<GpuSpectrogram>(*track.spectrogram, track.audio_buffer->Samplerate());
        tracks.push_back(std::move(track));
        zoom_window.LoadFile(length);
    }
}

void State::LoadQueuedFiles() {
    for (const std::string& file_name : files_to_load) {
        LoadFile(file_name);
    }
    files_to_load.clear();
}

void State::UnloadFiles() {
    tracks.clear();
}

void State::UnloadSelectedTrack() {
    if (selected_track) {
        tracks.erase(tracks.begin() + *selected_track);
        float max_len = 0.f;
        for (const Track& t : tracks) {
            max_len = std::max(max_len, t.audio_buffer->Length());
        }
        zoom_window.UnloadFile(max_len);
        if (tracks.size()) {
            selected_track = std::min(*selected_track, static_cast<int>(tracks.size() - 1));
        } else {
            selected_track.reset();
        }
    }
}

void State::TogglePlayback() {
    if (SelectedTrack()) {
        audio->TogglePlayback(tracks[*selected_track].audio_buffer, Cursor(), Selection());
        last_played_track = *selected_track;
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
