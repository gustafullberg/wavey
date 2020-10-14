#include "state.hpp"

void State::LoadFile(std::string file_name) {
    std::shared_ptr<AudioBuffer> ab = std::make_shared<AudioBuffer>();
    ab->LoadFile(file_name);
    if (*ab) {
        float length = ab->Length();
        Track track;
        track.path = file_name;
        track.audio_buffer = std::move(ab);
        track.spectrogram = std::make_unique<Spectrogram>(track.audio_buffer->Samples(),
                                                          track.audio_buffer->NumChannels(),
                                                          track.audio_buffer->NumFrames());
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
        if (!t.gpu_waveform) {
            t.gpu_waveform = std::make_unique<GpuWaveform>(*t.audio_buffer);
        }
        if (!t.gpu_spectrogram) {
            t.gpu_spectrogram =
                std::make_unique<GpuSpectrogram>(*t.spectrogram, t.audio_buffer->Samplerate());
        }
    }
}

void State::DeleteGpuBuffers() {
    for (Track& t : tracks) {
        if (t.gpu_waveform) {
            t.gpu_waveform.reset();
        }
        if (t.gpu_spectrogram) {
            t.gpu_spectrogram.reset();
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
