#include "state.hpp"
#include <sys/stat.h>
#include <fstream>

namespace {
uint64_t GetModTime(std::string path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return 1000000000 * statbuf.st_mtim.tv_sec + statbuf.st_mtim.tv_nsec;
    }
    return 0;
}
}  // namespace

void State::LoadFile(const std::string& file_name) {
    if (file_name.size() >= 4 && file_name.substr(file_name.size() - 4).compare(".lof") == 0) {
        return LoadListOfFiles(file_name);
    }

    Track track;
    track.path = file_name;
    const size_t separator_pos = file_name.rfind('/');
    track.short_name =
        separator_pos != std::string::npos ? file_name.substr(separator_pos + 1) : file_name;
    track.mod_time = GetModTime(track.path);
    tracks.push_back(std::move(track));

    // Make sure a track is selected.
    if (tracks.size() && !selected_track) {
        selected_track = 0;
    }
}

void State::LoadListOfFiles(const std::string& file_name) {
    // Get directory of lof file.
    std::string dir = "";
    size_t last_sep = file_name.find_last_of('/');
    if (last_sep != std::string::npos) {
        dir = file_name.substr(0, last_sep + 1);
    }

    // Scan file for file names.
    std::ifstream infile(file_name);
    std::string line;
    while (std::getline(infile, line)) {
        if (line.size() >= 8 && line.substr(0, 6).compare("file \"") == 0) {
            size_t start = 6;
            size_t end = line.find('\"', start);
            if (end != std::string::npos) {
                std::string path = line.substr(start, end - start);
                // Prepend dir to path unless path starts with '/'.
                if (path.size() >= 1 && path[0] != '/') {
                    path = dir + path;
                }

                // Load.
                LoadFile(path);
            }
        }
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
        uint64_t mod_time = GetModTime(t.path);
        if (mod_time != t.mod_time) {
            t.mod_time = mod_time;
            t.reload = true;
        }
    }
}

bool State::CreateResources(bool* view_reset) {
    *view_reset = false;
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
                float view_start = zoom_window.Left();
                float view_end = zoom_window.Right();
                ResetView();
                zoom_window.ZoomRange(view_start, view_end);
                *view_reset = true;
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
                t.channel_labels.resize(0);
                t.gpu_waveform.reset();
                t.gpu_spectrogram.reset();
                t.gpu_track_label.reset();
                t.gpu_channel_labels.resize(0);
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
                    t.channel_labels.resize(t.audio_buffer->NumChannels());
                    t.gpu_channel_labels.resize(t.audio_buffer->NumChannels());
                    ResetView();
                    *view_reset = true;
                }
            }
        }

        // Create track label.
        if (!t.label && t.audio_buffer) {
            t.label = TrackLabel::CreateTrackLabel(
                t.path, t.short_name, t.audio_buffer->NumChannels(), t.audio_buffer->Samplerate());
        }

        // Create channel lables.
        for (size_t i = 0; i < t.channel_labels.size(); ++i) {
            if (!t.channel_labels[i]) {
                t.channel_labels[i] = TrackLabel::CreateChannelLabel(t.path, t.short_name, i + 1,
                                                                     t.audio_buffer->NumChannels(),
                                                                     t.audio_buffer->Samplerate());
            }
        }

        // Create spectrogram.
        if (!t.spectrogram && t.audio_buffer && !t.gpu_spectrogram) {
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
            t.spectrogram.reset();
        }

        // Create GPU representation of track label.
        if (!t.gpu_track_label && t.label && t.label->HasImageData()) {
            t.gpu_track_label = std::make_unique<GpuTrackLabel>(
                t.label->ImageData(), t.label->Width(), t.label->Height());
        }

        // Create GPU representations of channel labels.
        bool gpu_channel_labels_loaded = true;
        for (size_t i = 0; i < t.gpu_channel_labels.size(); ++i) {
            if (!t.gpu_channel_labels[i]) {
                if (t.channel_labels[i] && t.channel_labels[i]->HasImageData()) {
                    t.gpu_channel_labels[i] = std::make_unique<GpuTrackLabel>(
                        t.channel_labels[i]->ImageData(), t.channel_labels[i]->Width(),
                        t.channel_labels[i]->Height());
                } else {
                    gpu_channel_labels_loaded = false;
                }
            }
        }

        all_resources_loaded = all_resources_loaded && t.gpu_waveform && t.gpu_spectrogram &&
                               t.gpu_track_label && gpu_channel_labels_loaded;
    }

    return all_resources_loaded;
}

void State::SetLooping(bool do_loop) {
    audio->SetLooping(do_loop);
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
    track = std::max(0, std::min(static_cast<int>(tracks.size()) - 1, track));
    if (!tracks.size()) {
        selected_track.reset();
        return false;
    } else if (selected_track && *selected_track == track)
        return false;
    else {
        selected_track = track;
        if (view_mode == TRACK) {
            zoom_window.ShowSingleTrack(track);
        }
    }
    return true;
}

Track& State::GetTrack(int number) {
    return *std::next(tracks.begin(), number);
}

Track& State::GetSelectedTrack() {
    return GetTrack(*selected_track);
}

void State::ResetView() {
    view_mode = ALL;
    selected_channel.reset();
    zoom_window.Reset();
    if (tracks.size()) {
        for (Track& t : tracks) {
            float length = t.audio_buffer ? t.audio_buffer->Duration() : 0.f;
            zoom_window.LoadFile(length);
        }
    }
}

int State::GetCurrentSamplerate() {
    return GetSelectedTrack().GetSamplerate();
}

void State::ToggleViewSingleTrack() {
    selected_channel.reset();
    if (view_mode == ALL && selected_track) {
        view_mode = TRACK;
        zoom_window.ShowSingleTrack(selected_track);
    } else {
        view_mode = ALL;
        zoom_window.ShowAllTracks();
    }
}

void State::ToggleViewSingleChannel(float mouse_y) {
    if (!selected_track || !GetSelectedTrack().audio_buffer) {
        return;
    }

    int num_channels = GetSelectedTrack().audio_buffer->NumChannels();
    if (view_mode == CHANNEL || num_channels == 1) {
        selected_channel.reset();
        view_mode = TRACK;
        zoom_window.ShowSingleTrack(selected_track);
    } else {
        int hover_track = zoom_window.GetTrack(mouse_y);
        if (hover_track == *selected_track) {
            selected_channel = zoom_window.GetChannel(mouse_y, num_channels);
        } else {
            selected_channel = 0;
        }
        view_mode = CHANNEL;
        zoom_window.ShowSingleChannel(*selected_track, *selected_channel, num_channels);
    }
}

void State::ScrollUp() {
    if (view_mode == CHANNEL) {
        if (selected_track && selected_channel && GetSelectedTrack().audio_buffer &&
            *selected_channel > 0) {
            selected_channel = *selected_channel - 1;
            zoom_window.ShowSingleChannel(*selected_track, *selected_channel,
                                          GetSelectedTrack().audio_buffer->NumChannels());
        }
    } else {
        if (selected_track && *selected_track > 0) {
            selected_track = *selected_track - 1;
            if (view_mode == TRACK) {
                zoom_window.ShowSingleTrack(selected_track);
            }
        }
    }
}

void State::ScrollDown() {
    if (view_mode == CHANNEL) {
        if (selected_track && selected_channel && GetSelectedTrack().audio_buffer &&
            *selected_channel < GetSelectedTrack().audio_buffer->NumChannels() - 1) {
            selected_channel = *selected_channel + 1;
            zoom_window.ShowSingleChannel(*selected_track, *selected_channel,
                                          GetSelectedTrack().audio_buffer->NumChannels());
        }
    } else {
        if (selected_track && *selected_track < static_cast<int>(tracks.size()) - 1) {
            selected_track = *selected_track + 1;
            if (view_mode == TRACK) {
                zoom_window.ShowSingleTrack(selected_track);
            }
        }
    }
}

void State::MoveTrackUp() {
    if (selected_track && *selected_track > 0) {
        if (last_played_track == *selected_track) {
            last_played_track--;
        } else if (last_played_track == *selected_track - 1) {
            last_played_track++;
        }
        selected_track = *selected_track - 1;
        std::swap(*std::next(tracks.begin(), *selected_track),
                  *std::next(tracks.begin(), *selected_track + 1));
    }
}

void State::MoveTrackDown() {
    if (selected_track && *selected_track < tracks.size() - 1) {
        if (last_played_track == *selected_track) {
            last_played_track++;
        } else if (last_played_track == *selected_track + 1) {
            last_played_track--;
        }
        std::swap(*std::next(tracks.begin(), *selected_track),
                  *std::next(tracks.begin(), *selected_track + 1));
        selected_track = *selected_track + 1;
    }
}
