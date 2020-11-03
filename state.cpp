#include "state.hpp"

void State::LoadFile(const std::string& file_name) {
    Track track;
    track.path = file_name;
    const size_t separator_pos = file_name.rfind('/');
    track.short_name =
        separator_pos != std::string::npos ? file_name.substr(separator_pos + 1) : file_name;
    tracks.push_back(std::move(track));

    // Make sure a track is selected.
    if (tracks.size() && !selected_track) {
        selected_track = 0;
    }
}

void State::UnloadFiles() {
    // Make sure async work is completed.
    for (Track& t : tracks) {
        if (t.future_audio_buffer.valid())
            t.future_audio_buffer.wait();
        if (t.future_spectrogram.valid())
            t.future_spectrogram.wait();
    }

    tracks.clear();
    ResetView();
    selected_track.reset();
}

void State::UnloadSelectedTrack() {
    GetSelectedTrack().remove = true;
}

void State::ReloadFiles() {
    for (Track& t : tracks) {
        t.reload = true;
    }
}

bool State::CreateResources() {
    for (auto i = tracks.begin(); i != tracks.end();) {
        Track& t = *i;
        if (t.remove || t.reload) {
            // Make sure async work is completed.
            if (t.future_audio_buffer.valid())
                t.future_audio_buffer.wait();
            if (t.future_spectrogram.valid())
                t.future_spectrogram.wait();

            // Remove track.
            if (t.remove) {
                tracks.erase(i++);
                ResetView();
                if (selected_track && tracks.size()) {
                    selected_track = std::min(*selected_track, static_cast<int>(tracks.size()) - 1);
                } else {
                    selected_track.reset();
                }
                continue;
            }

            // Reload track.
            if (t.reload) {
                t.audio_buffer.reset();
                t.spectrogram.reset();
                t.label.reset();
                t.gpu_waveform.reset();
                t.gpu_spectrogram.reset();
                t.gpu_label.reset();
                t.reload = false;
            }
        }
        i++;
    }

    bool all_resources_loaded = true;
    for (Track& t : tracks) {
        // Create audio buffer.
        if (!t.audio_buffer) {
            if (!t.future_audio_buffer.valid()) {
                // Asynchronous creation of audio buffer.
                t.future_audio_buffer =
                    std::async([&t] { return std::make_shared<AudioBuffer>(t.path); });
            } else {
                // Check if audio buffer is ready.
                if (t.future_audio_buffer.wait_for(std::chrono::seconds(0)) ==
                    std::future_status::ready) {
                    t.audio_buffer = t.future_audio_buffer.get();
                    ResetView();
                }
            }
        }

        // Create track label.
        if (!t.label && t.audio_buffer) {
            t.label = TrackLabel::Create(t.path, t.short_name, t.audio_buffer->NumChannels(),
                                         t.audio_buffer->Samplerate());
        }

        // Create spectrogram.
        if (!t.spectrogram && t.audio_buffer) {
            if (!t.future_spectrogram.valid()) {
                // Asynchronous creation of spectrogram.
                t.future_spectrogram = std::async([&t] {
                    return std::make_unique<Spectrogram>(t.audio_buffer->Samples(),
                                                         t.audio_buffer->NumChannels(),
                                                         t.audio_buffer->NumFrames());
                });
            } else {
                // Check if spectrogram is ready.
                if (t.future_spectrogram.wait_for(std::chrono::seconds(0)) ==
                    std::future_status::ready) {
                    t.spectrogram = t.future_spectrogram.get();
                }
            }
        }

        // Create GPU representation of waveform.
        if (!t.gpu_waveform && t.audio_buffer) {
            t.gpu_waveform = std::make_unique<GpuWaveform>(*t.audio_buffer);
        }

        // Create GPU representation of spectrogram.
        if (!t.gpu_spectrogram && t.spectrogram) {
            t.gpu_spectrogram =
                std::make_unique<GpuSpectrogram>(*t.spectrogram, t.audio_buffer->Samplerate());
        }

        // Create GPU representation of track label.
        if (!t.gpu_label && t.label && t.label->HasImageData()) {
            t.gpu_label = std::make_unique<GpuTrackLabel>(t.label->ImageData(), t.label->Width(),
                                                          t.label->Height());
        }

        all_resources_loaded =
            all_resources_loaded && t.gpu_waveform && t.gpu_spectrogram && t.gpu_label;
    }

    return all_resources_loaded;
}

void State::TogglePlayback() {
    if (SelectedTrack()) {
        Track& t = GetTrack(*selected_track);
        if (t.audio_buffer) {
            audio->TogglePlayback(t.audio_buffer, Cursor(), Selection());
            last_played_track = *selected_track;
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

Track& State::GetTrack(int number) {
    return *std::next(tracks.begin(), number);
}

Track& State::GetSelectedTrack() {
    return GetTrack(*selected_track);
}

void State::ResetView() {
    zoom_window.Reset();
    if (tracks.size()) {
        for (Track& t : tracks) {
            float length = t.audio_buffer ? t.audio_buffer->Length() : 0.f;
            zoom_window.LoadFile(length);
        }
    }
}
