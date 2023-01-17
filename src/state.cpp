#include "state.hpp"
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <optional>

#include "audio_mixer.hpp"
#include "file_notification.hpp"

namespace {
uint64_t GetModTime(std::string path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) == 0) {
        return 1000000000 * statbuf.st_mtim.tv_sec + statbuf.st_mtim.tv_nsec;
    }
    return 0;
}

std::optional<std::string> GetLabelFromLofOptionString(const std::string& options) {
    std::string::size_type n = options.find("label \"");
    if (n == std::string::npos) {
        return std::nullopt;
    }
    const std::string::size_type start = n + 7;
    const std::string::size_type end = options.find("\"", start);
    std::string label = options.substr(start, end - start);
    return label;
}

}  // namespace

Track::Track(const std::string& filename, std::optional<std::string> track_label) : path(filename) {
    const size_t separator_pos = path.rfind('/');
    std::string base_filename =
        separator_pos != std::string::npos ? path.substr(separator_pos + 1) : path;
    if (track_label.has_value()) {
        short_name = *track_label + " <i>[" + base_filename + "]</i>";
    } else {
        short_name = base_filename;
    }
    mod_time = GetModTime(path);
}

void Track::Reload() {
    uint64_t current_mod_time = GetModTime(path);
    if (mod_time != current_mod_time) {
        mod_time = current_mod_time;
        reload = true;
    }
}

void State::LoadFile(const std::string& file_name, std::optional<std::string> label) {
    if (file_name.size() >= 4 && file_name.substr(file_name.size() - 4).compare(".lof") == 0) {
        return LoadListOfFiles(file_name);
    }

    Track track(file_name, label);
    if (track_change_notifier_) {
        MonitorTrack(track);
    }
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
            size_t end_filename = line.find('\"', start);
            if (end_filename != std::string::npos) {
                std::string path = line.substr(start, end_filename - start);
                // Prepend dir to path unless path starts with '/'.
                if (path.size() >= 1 && path[0] != '/') {
                    path = dir + path;
                }

                std::string options_string = line.substr(end_filename, line.size());

                // Load.
                LoadFile(path, GetLabelFromLofOptionString(options_string));
            }
        }
    }
}

void State::UnloadFiles() {
    // Make sure async work is completed.
    for (Track& t : tracks) {
        if (t.future_audio_buffer.valid())
            t.future_audio_buffer.wait();
        if (t.future_lowres_waveform.valid())
            t.future_lowres_waveform.wait();
        if (t.future_spectrogram.valid())
            t.future_spectrogram.wait();
    }

    tracks.clear();
    ResetView();
}

void State::UnloadSelectedTrack() {
    if (selected_track) {
        Track& t = GetSelectedTrack();
        t.remove = true;
        if (track_change_notifier_ && t.watch_id_) {
            track_change_notifier_->Unwatch(*t.watch_id_);
        }
    }
}

void State::ReloadFiles() {
    for (Track& t : tracks) {
        t.Reload();
    }
}

void State::MonitorTrack(Track& t) {
    int wd = track_change_notifier_->Watch(t.path);
    t.watch_id_ = wd;
}

void State::UnmonitorTrack(Track& t) {
    track_change_notifier_->Unwatch(*t.watch_id_);
    t.watch_id_.reset();
}

void State::StartMonitoringTrackChange() {
    track_change_notifier_.emplace([this](int id) {
        for (Track& t : tracks) {
            if (t.watch_id_ == id) {
                t.Reload();
            }
        }
    });
    for (Track& t : tracks) {
        MonitorTrack(t);
    }
}
void State::StopMonitoringTrackChange() {
    track_change_notifier_.reset();
    for (Track& t : tracks) {
        t.watch_id_.reset();
    }
}

bool State::CreateResources() {
    for (auto i = tracks.begin(); i != tracks.end();) {
        Track& t = *i;
        if (t.remove || t.reload) {
            // Make sure async work is completed.
            if (t.future_audio_buffer.valid())
                t.future_audio_buffer.get();
            if (t.future_lowres_waveform.valid())
                t.future_lowres_waveform.get();
            if (t.future_spectrogram.valid())
                t.future_spectrogram.get();

            // Remove track.
            if (t.remove) {
                tracks.erase(i++);
                ResetView();
                continue;
            }

            // Reload track.
            if (t.reload) {
                t.future_audio_buffer =
                    std::async([&t] { return std::make_shared<AudioBuffer>(t.path); });
                t.reload = false;
            }
        }
        i++;
    }

    bool resources_to_load = false;
    for (Track& t : tracks) {
        resources_to_load =
            resources_to_load || !t.audio_buffer || !t.gpu_waveform || !t.gpu_spectrogram;

        // Asynchronous creation of audio buffer.
        if (!t.audio_buffer && !t.future_audio_buffer.valid()) {
            t.status = "Loading: " + t.path;
            t.future_audio_buffer =
                std::async([&t] { return std::make_shared<AudioBuffer>(t.path); });
            ResetView();
        }

        // Check if new audio buffer is loaded.
        if (t.future_audio_buffer.valid()) {
            resources_to_load = true;
            if (t.future_audio_buffer.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready) {
                t.audio_buffer = t.future_audio_buffer.get();
                t.spectrogram.reset();
                t.gpu_waveform.reset();
                t.gpu_spectrogram.reset();

                if (t.audio_buffer->NumChannels() == 0) {
                    t.status = "Failed to load: " + t.path;
                }
                ResetView();
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
            if (!t.future_lowres_waveform.valid()) {
                // Asynchronous creation of low-res waveform.
                t.future_lowres_waveform =
                    std::async([&t] { return std::make_unique<LowResWaveform>(*t.audio_buffer); });
            } else {
                if (t.future_lowres_waveform.wait_for(std::chrono::seconds(0)) ==
                    std::future_status::ready) {
                    std::unique_ptr<LowResWaveform> lowres_waveform =
                        t.future_lowres_waveform.get();
                    t.gpu_waveform = std::make_unique<GpuWaveform>(*t.audio_buffer,
                                                                   lowres_waveform->GetBuffer());
                    // Indicate that file is loaded.
                    if (t.audio_buffer->NumChannels()) {
                        t.status = "";
                    }
                }
            }
        }

        // Create GPU representation of spectrogram.
        if (!t.gpu_spectrogram && t.spectrogram) {
            t.gpu_spectrogram =
                std::make_unique<GpuSpectrogram>(*t.spectrogram, t.audio_buffer->Samplerate());
            t.spectrogram.reset();
        }
    }

    return resources_to_load;
}

void State::SetLooping(bool do_loop) {
    audio->SetLooping(do_loop);
}

void State::TogglePlayback() {
    if (SelectedTrack()) {
        Track& t = GetTrack(*selected_track);
        if (t.audio_buffer) {
            std::unique_ptr<AudioMixer> mixer = std::make_unique<AudioMixer>(
                t.audio_buffer->NumChannels(), audio->NumOutputChannels());
            if (t.selected_channel) {
                mixer->Solo(*t.selected_channel);
            }
            audio->TogglePlayback(t.audio_buffer, std::move(mixer), Cursor(), Selection());
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
    // Remember zoom time interval before reset.
    const float view_start = zoom_window.Left();
    const float view_end = zoom_window.Right();
    const bool restore_view = view_start != 0.f || view_end != zoom_window.MaxX();

    // Make sure selected_track dows not exceed its maximum value.
    if (selected_track && tracks.size()) {
        selected_track = std::min(*selected_track, static_cast<int>(tracks.size()) - 1);
    } else {
        selected_track.reset();
    }

    // Make sure selected_channel does not exceed its maximum value.
    for (Track& t : tracks) {
        if (t.selected_channel && t.audio_buffer) {
            t.selected_channel = std::min(*t.selected_channel, t.audio_buffer->NumChannels() - 1);
        } else {
            t.selected_channel.reset();
        }
    }

    zoom_window.Reset();
    if (tracks.size()) {
        for (Track& t : tracks) {
            float length = t.audio_buffer ? t.audio_buffer->Duration() : 0.f;
            zoom_window.LoadFile(length);
        }
    }

    if (restore_view) {
        zoom_window.ZoomRange(view_start, view_end);
    }

    if (view_mode == TRACK) {
        if (selected_track) {
            zoom_window.ShowSingleTrack(selected_track);
        } else {
            view_mode = ALL;
        }
    }
}

int State::GetCurrentSamplerate() {
    return GetSelectedTrack().GetSamplerate();
}

void State::ToggleViewSingleTrack() {
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

    Track& current_track = GetSelectedTrack();
    if (current_track.selected_channel) {
        current_track.selected_channel.reset();
        return;
    }

    int num_channels = current_track.audio_buffer->NumChannels();
    if (num_channels == 1) {
        current_track.selected_channel.reset();
        return;
    }
    int hover_track = zoom_window.GetTrack(mouse_y);
    if (hover_track == *selected_track) {
        current_track.selected_channel = zoom_window.GetChannel(mouse_y, num_channels);
    } else {
        current_track.selected_channel = 0;
    }
}

void State::ScrollTrackUp() {
    if (view_mode == TRACK && GetSelectedTrack().selected_channel) {
        ScrollChannelUp();
    } else {
        if (selected_track && *selected_track > 0) {
            selected_track = *selected_track - 1;
            if (view_mode == TRACK) {
                zoom_window.ShowSingleTrack(selected_track);
            }
        }
    }
}

void State::ScrollTrackDown() {
    if (view_mode == TRACK && GetSelectedTrack().selected_channel) {
        ScrollChannelDown();
    } else {
        if (selected_track && *selected_track < static_cast<int>(tracks.size()) - 1) {
            selected_track = *selected_track + 1;
            if (view_mode == TRACK) {
                zoom_window.ShowSingleTrack(selected_track);
            }
        }
    }
}

void State::ScrollChannelUp() {
    if (!GetSelectedTrack().selected_channel) {
        return;
    }
    GetSelectedTrack().selected_channel =
        std::max(0, GetSelectedTrack().selected_channel.value() - 1);
}

void State::ScrollChannelDown() {
    if (!GetSelectedTrack().selected_channel) {
        return;
    }
    GetSelectedTrack().selected_channel =
        std::min(GetSelectedTrack().audio_buffer->NumChannels() - 1,
                 GetSelectedTrack().selected_channel.value() + 1);
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
    if (selected_track && *selected_track < static_cast<int>(tracks.size()) - 1) {
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
