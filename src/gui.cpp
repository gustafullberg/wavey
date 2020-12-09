#include "gui.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <iostream>  //DEBUG

namespace {

Glib::ustring FormatTime(float t, bool show_minutes = true) {
    float minutes = std::floor(t / 60.f);
    float seconds = t - 60.f * minutes;
    if (show_minutes || minutes > 0.f) {
        return Glib::ustring::sprintf("%02.f:%06.03f", minutes, seconds);
    } else {
        return Glib::ustring::sprintf("%.03f s", seconds);
    }
}

Glib::ustring FormatSelectionDuration(float duration, int samplerate) {
    Glib::ustring duration_status =
        FormatTime(duration, /*show_minutes=*/false) +
        Glib::ustring::sprintf(" - %d samples", static_cast<int>(duration * samplerate));
    if (duration > 1e-5f && duration < 1e5f) {
        float frequency = 1.f / duration;
        duration_status += Glib::ustring::sprintf(" - %.03f Hz", frequency);
    }
    return duration_status;
}
}  // namespace

Gui::Gui(State* state) : state(state) {
    signal_key_press_event().connect(sigc::mem_fun(*this, &Gui::KeyPress), false);
    add_events(Gdk::KEY_PRESS_MASK);

    set_default_size(800, 600);

    set_titlebar(headerbar);
    headerbar.set_has_subtitle(false);
    headerbar.set_show_close_button();
    open.set_label("_Open");
    open.set_use_underline();
    open.signal_clicked().connect(sigc::mem_fun(*this, &Gui::ChooseFiles));
    headerbar.add(open);

    add(box);

    box.pack_start(grid_top, false, true);
    grid_top.set_row_homogeneous(true);
    grid_top.set_column_homogeneous(true);
    grid_top.attach(status_view_start, 0, 0, 1, 1);
    grid_top.attach(status_view_length, 1, 0, 1, 1);
    grid_top.attach(status_view_end, 2, 0, 1, 1);
    status_view_start.set_xalign(0);
    status_view_start.set_margin_left(5);
    status_view_length.set_xalign(0.5f);
    status_view_end.set_xalign(1);
    status_view_end.set_margin_right(5);

    glarea.set_required_version(3, 3);
    glarea.signal_realize().connect(sigc::mem_fun(*this, &Gui::Realize));
    glarea.signal_unrealize().connect(sigc::mem_fun(*this, &Gui::Unrealize));
    glarea.signal_render().connect(sigc::mem_fun(*this, &Gui::Render));
    glarea.signal_resize().connect(sigc::mem_fun(*this, &Gui::Resize));
    glarea.signal_button_press_event().connect(sigc::mem_fun(*this, &Gui::ButtonPress));
    glarea.signal_button_release_event().connect(sigc::mem_fun(*this, &Gui::ButtonRelease));
    glarea.signal_motion_notify_event().connect(sigc::mem_fun(*this, &Gui::PointerMove));
    glarea.signal_scroll_event().connect(sigc::mem_fun(*this, &Gui::ScrollWheel));
    glarea.add_events(Gdk::BUTTON_PRESS_MASK);
    glarea.add_events(Gdk::BUTTON_RELEASE_MASK);
    glarea.add_events(Gdk::POINTER_MOTION_MASK);
    glarea.add_events(Gdk::SCROLL_MASK);
    box.pack_start(glarea);

    box.pack_start(scrollbar, false, false);
    scrollbar.signal_value_changed().connect(sigc::mem_fun(*this, &Gui::Scrolling));

    box.pack_start(grid_bottom, false, true);
    grid_bottom.set_row_homogeneous(true);
    grid_bottom.attach(status_time, 0, 0, 1, 1);
    grid_bottom.attach(status_frequency, 1, 0, 1, 1);
    status_time.set_xalign(0);
    status_time.set_margin_left(5);
    status_time.set_margin_right(5);
    status_frequency.set_hexpand(true);
    status_frequency.set_xalign(1);
    status_frequency.set_margin_left(5);
    status_frequency.set_margin_right(5);

    show_all();
    UpdateWidgets();
}

void Gui::Realize() {
    glarea.make_current();
    wave_shader.Init();
    spectrogram_shader.Init();
    prim_renderer.Init();
    label_renderer.Init();

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Gui::Unrealize() {
    state->UnloadFiles();
    wave_shader.Terminate();
    spectrogram_shader.Terminate();
    prim_renderer.Terminate();
    label_renderer.Terminate();
}

bool Gui::Render(const Glib::RefPtr<Gdk::GLContext> context) {
    bool view_reset;
    bool all_resources_loaded = state->CreateResources(&view_reset);
    if (view_reset) {
        UpdateWidgets();
    }
    const float scale = get_scale_factor();
    glClear(GL_COLOR_BUFFER_BIT);

    float play_time;
    bool playing = state->Playing(&play_time);

    const ZoomWindow& z = state->zoom_window;
    glm::mat4 mvp = glm::ortho(z.Left(), z.Right(), z.Bottom(), z.Top(), -1.f, 1.f);

    for (int i = 0; i < static_cast<int>(state->tracks.size()); i++) {
        const Track& t = state->GetTrack(i);
        if (!t.audio_buffer) {
            continue;
        }

        if (state->SelectedTrack() && i == state->SelectedTrack()) {
            glm::vec4 color_selection(.5f, .9f, .5f, .1f);
            prim_renderer.DrawQuad(mvp, glm::vec2(z.Left(), i + 1), glm::vec2(z.Right(), i),
                                   color_selection);
        }

        const int num_channels = t.audio_buffer->NumChannels();
        const int samplerate = t.audio_buffer->Samplerate();
        const float length = t.audio_buffer->Length();

        for (int c = 0; c < num_channels; c++) {
            const float trackOffset = i;
            const float channelOffset = (2.f * c + 1.f) / (2.f * num_channels);
            glm::mat4 mvp_channel =
                glm::translate(mvp, glm::vec3(0.f, trackOffset + channelOffset, 0.f));
            mvp_channel = glm::scale(mvp_channel, glm::vec3(1.f, -0.5f / num_channels, 1.f));

            glm::vec4 color_line(.5f, .9f, .5f, .1f);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 1.f), glm::vec2(length, 1.f),
                                   color_line);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 0.f), glm::vec2(length, 0.f),
                                   color_line);
            if (view_spectrogram) {
                if (t.gpu_spectrogram) {
                    spectrogram_shader.Draw(mvp_channel, samplerate, view_bark_scale);
                    t.gpu_spectrogram->Draw(c);
                }
            } else {
                float samples_per_pixel = (z.Right() - z.Left()) * samplerate / win_width;
                const bool use_low_res = samples_per_pixel > 1000.f;
                const float rate = use_low_res ? samplerate * 2.f / 1000.f : samplerate;
                wave_shader.Draw(mvp_channel, rate, z.VerticalZoom());
                t.gpu_waveform->Draw(c, z.Left(), z.Right(), use_low_res);
            }
        }

        if (t.gpu_label) {
            float y = std::round(win_height * (i - z.Top()) / (z.Bottom() - z.Top()));
            bool selected = state->SelectedTrack() && *state->SelectedTrack() == i;
            label_renderer.Draw(*t.gpu_label, y, win_width, win_height, scale, selected);
        }
    }

    // Selection.
    if (state->Selection()) {
        glm::vec4 color_selection(.5f, .9f, .5f, .1f);
        prim_renderer.DrawQuad(mvp, glm::vec2(state->Cursor(), z.Bottom()),
                               glm::vec2(*state->Selection(), z.Top()), color_selection);
    }

    // Cursor.
    glm::vec4 color_cursor(.5f, .9f, .5f, .5f);
    prim_renderer.DrawLine(mvp, glm::vec2(state->Cursor(), z.Bottom()),
                           glm::vec2(state->Cursor(), z.Top()), color_cursor);

    // Play position indicator.
    if (playing) {
        glm::vec4 color_play_indicator(.9f, .5f, .5f, 1.f);
        prim_renderer.DrawLine(mvp, glm::vec2(play_time, state->last_played_track + 1),
                               glm::vec2(play_time, state->last_played_track),
                               color_play_indicator);
    }

    // Redraw continuously until all resources are loaded and if playing audio.
    if (!all_resources_loaded || playing) {
        queue_draw();
    }
    return true;
}

void Gui::Resize(int width, int height) {
    win_width = width;
    win_height = height;
    queue_draw();
}

bool Gui::KeyPress(GdkEventKey* key_event) {
    bool ctrl = (key_event->state & Gtk::AccelGroup::get_default_mod_mask()) == Gdk::CONTROL_MASK;

    // Quit.
    if (key_event->keyval == GDK_KEY_q && ctrl) {
        close();
    }

    // Open files.
    if (key_event->keyval == GDK_KEY_o && ctrl) {
        ChooseFiles();
    }

    // Full zoom out.
    if (key_event->keyval == GDK_KEY_f && ctrl) {
        state->zoom_window.ZoomOutFull();
    }

    // Zoom to selection.
    if (key_event->keyval == GDK_KEY_e && ctrl && state->Selection()) {
        state->zoom_window.ZoomRange(state->Cursor(), *state->Selection());
    }

    // Zoom toggle one/all tracks.
    if (key_event->keyval == GDK_KEY_z) {
        state->zoom_window.ToggleSingleTrack(state->SelectedTrack());
    }

    // Zoom in.
    if (key_event->keyval == GDK_KEY_plus || key_event->keyval == GDK_KEY_KP_Add) {
        if (ctrl) {
            state->zoom_window.ZoomInVertical();
        } else {
            state->zoom_window.ZoomIn(.5f);
        }
    }

    // Zoom out.
    if (key_event->keyval == GDK_KEY_minus || key_event->keyval == GDK_KEY_KP_Subtract) {
        if (ctrl) {
            state->zoom_window.ZoomOutVertical();
        } else {
            state->zoom_window.ZoomOut(.5f);
        }
    }

    // Reset vertical zoom.
    if (key_event->keyval == GDK_KEY_0 && ctrl) {
        state->zoom_window.ZoomOutFullVertical();
    }

    // Scroll and move the cursor to the beginning.
    if (key_event->keyval == GDK_KEY_Home) {
        state->SetCursor(0.f);
        scrollbar.set_value(0.f);
    }

    // Scroll and move the cursor to the end.
    if (key_event->keyval == GDK_KEY_End) {
        auto adjustment = scrollbar.get_adjustment();
        state->SetCursor(adjustment->get_upper());
        scrollbar.set_value(adjustment->get_upper());
    }

    // Pan and select track with arrow keys.
    if (key_event->keyval == GDK_KEY_Left) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() - adjustment->get_step_increment());
    } else if (key_event->keyval == GDK_KEY_Right) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() + adjustment->get_step_increment());
    } else if (key_event->keyval == GDK_KEY_Up) {
        if (state->SelectedTrack()) {
            state->SetSelectedTrack(*state->SelectedTrack() - 1);
        }
    } else if (key_event->keyval == GDK_KEY_Down) {
        if (state->SelectedTrack()) {
            state->SetSelectedTrack(*state->SelectedTrack() + 1);
        }
    }

    // Toggle spectrogram view.
    if (key_event->keyval == GDK_KEY_s) {
        view_spectrogram = !view_spectrogram;
    }

    // Toggle bark scale spectrograms.
    if (key_event->keyval == GDK_KEY_b) {
        if (view_spectrogram) {
            view_bark_scale = !view_bark_scale;
        }
    }

    // Start/stop playback.
    if (key_event->keyval == GDK_KEY_space) {
        state->TogglePlayback();
        float time;
        if (state->Playing(&time)) {
            StartTimeUpdate();
            queue_draw();
        }
        return true;
    }

    // Close selected track.
    if (key_event->keyval == GDK_KEY_w && ctrl) {
        if (!state->tracks.size()) {
            close();
        } else {
            state->UnloadSelectedTrack();
            queue_draw();
        }
    }

    // Reload all files.
    if (key_event->keyval == GDK_KEY_r && ctrl) {
        state->ReloadFiles();
        queue_draw();
    }

    UpdateWidgets();
    queue_draw();
    return false;
}

bool Gui::ButtonPress(GdkEventButton* button_event) {
    const float scale = get_scale_factor();
    const float x = button_event->x * scale / win_width;

    if (button_event->button == 1) {
        if (button_event->type == GDK_BUTTON_PRESS) {
            const float time = state->zoom_window.GetTime(x);
            state->SetCursor(time);
            mouse_down = true;
            queue_draw();
        } else if (button_event->type == GDK_2BUTTON_PRESS) {
            if (state->SelectedTrack() && state->GetSelectedTrack().audio_buffer) {
                state->SetCursor(0.f);
                state->SetSelection(state->GetSelectedTrack().audio_buffer->Length());
                queue_draw();
            }
        }
        UpdateWidgets();
    }
    return true;
}

bool Gui::ButtonRelease(GdkEventButton* button_event) {
    if (button_event->button == 1) {
        if (button_event->type == GDK_BUTTON_RELEASE) {
            state->FixSelection();
            UpdateWidgets();
            queue_draw();
        }
        mouse_down = false;
    }
    return true;
}

bool Gui::PointerMove(GdkEventMotion* motion_event) {
    const float scale = get_scale_factor();
    mouse_x = std::min(std::max(static_cast<float>(motion_event->x) * scale / win_width, 0.f), 1.f);
    mouse_y =
        std::min(std::max(static_cast<float>(motion_event->y) * scale / win_height, 0.f), 1.f);

    if (mouse_down) {
        ZoomWindow& z = state->zoom_window;
        const float time = z.GetTime(mouse_x);
        const float dt = std::fabs(time - state->Cursor());
        // Mouse needs to move at least one pixel to count as a interval selection.
        if (dt >= (z.Right() - z.Left()) / win_width) {
            state->SetSelection(time);
            queue_draw();
        }
    }

    bool changed = state->SetSelectedTrack(state->zoom_window.GetTrack(mouse_y));
    if (changed) {
        queue_draw();
    }

    UpdateWidgets();

    return true;
}

bool Gui::ScrollWheel(GdkEventScroll* scroll_event) {
    const float scale = get_scale_factor();
    const float x = scroll_event->x * scale / win_width;
    bool ctrl =
        (scroll_event->state & Gtk::AccelGroup::get_default_mod_mask()) == Gdk::CONTROL_MASK;

    GdkScrollDirection dir = scroll_event->direction;
    if (dir == GDK_SCROLL_UP) {
        if (ctrl) {
            state->zoom_window.ZoomInVertical();
        } else {
            state->zoom_window.ZoomIn(x);
        }
    } else if (dir == GDK_SCROLL_DOWN) {
        if (ctrl) {
            state->zoom_window.ZoomOutVertical();
        } else {
            state->zoom_window.ZoomOut(x);
        }
    } else if (dir == GDK_SCROLL_LEFT) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() - adjustment->get_step_increment());
    } else if (dir == GDK_SCROLL_RIGHT) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() + adjustment->get_step_increment());
    }

    UpdateWidgets();
    queue_draw();
    return true;
}

void Gui::Scrolling() {
    auto adjustment = scrollbar.get_adjustment();
    state->zoom_window.PanTo(adjustment->get_value());
    UpdateWidgets();
    queue_draw();
}

void Gui::ChooseFiles() {
    Gtk::FileChooserDialog dialog("Open", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    dialog.set_select_multiple();
    if (open_dir.size()) {
        dialog.set_current_folder(open_dir);
    }
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        std::vector<std::string> files = dialog.get_filenames();
        for (std::string file : files) {
            state->LoadFile(file);
            size_t last_sep = file.find_last_of('/');
            if (last_sep != std::string::npos) {
                open_dir = file.substr(0, last_sep);
            }
        }
    }
}

void Gui::StartTimeUpdate() {
    UpdateTime();
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &Gui::UpdateTime), 20);
}

void Gui::UpdateWidgets() {
    UpdateTime();
    UpdateZoom();
    UpdateFrequency();
    UpdateTitle();
}

bool Gui::UpdateTime() {
    float play_time;
    bool playing = state->Playing(&play_time);
    float time;
    if (playing) {
        time = play_time;
    } else {
        time = state->Cursor();
    }

    Glib::ustring s;
    if (playing || !state->Selection()) {
        s = Glib::ustring::compose("<tt>Time: <b>%1</b></tt>", FormatTime(time));
    } else {
        float s_start = time;
        float s_end = *state->Selection();
        if (s_end < s_start)
            std::swap(s_start, s_end);

        const float s_duration = s_end - s_start;
        s = Glib::ustring::compose(
            "<tt>Time: %1 - %2 (%3)</tt>", FormatTime(s_start), FormatTime(s_end),
            FormatSelectionDuration(s_duration, state->GetCurrentSamplerate()));
    }

    if (s != str_time) {
        str_time = s;
        status_time.set_markup(str_time);
    }

    return playing;
}

void Gui::UpdateZoom() {
    const ZoomWindow& z = state->zoom_window;
    const float page_size = z.Right() - z.Left();
    auto adjustment = scrollbar.get_adjustment();
    adjustment->set_lower(0);
    adjustment->set_upper(z.MaxX());
    adjustment->set_page_size(page_size);
    adjustment->set_step_increment(0.1f * page_size);
    adjustment->set_page_increment(page_size);
    adjustment->set_value(z.Left());

    {
        Glib::ustring s = Glib::ustring::compose("<tt>%1</tt>", FormatTime(z.Left()));
        if (s != str_view_start) {
            str_view_start = s;
            status_view_start.set_markup(str_view_start);
        }
    }

    {
        Glib::ustring s = Glib::ustring::compose("<tt>%1</tt>", FormatTime(z.Right()));
        if (s != str_view_end) {
            str_view_end = s;
            status_view_end.set_markup(str_view_end);
        }
    }

    {
        Glib::ustring s =
            Glib::ustring::compose("<tt>(%1)</tt>", FormatTime(z.Right() - z.Left(), false));
        if (s != str_view_length) {
            str_view_length = s;
            status_view_length.set_markup(str_view_length);
        }
    }
}

void Gui::UpdateFrequency() {
    Glib::ustring s = "";
    if (state->tracks.size()) {
        ZoomWindow& z = state->zoom_window;
        int track_number = z.GetTrack(mouse_y);
        Track& t = state->GetTrack(track_number);
        if (t.audio_buffer) {
            // Track space: On what y-coordinate is the pointer?
            float y = 1.f - std::fmod(z.Top() + (z.Bottom() - z.Top()) * mouse_y, 1.f);
            // Channel space: On what y-coordinate is the pointer?
            float num_channels = t.audio_buffer->NumChannels();
            y = std::fmod(num_channels * y, 1.f);
            if (view_spectrogram) {
                float f = 0.f;
                float nyquist_freq = 0.5f * t.audio_buffer->Samplerate();
                if (view_bark_scale) {
                    const float bark_scaling =
                        26.81f * nyquist_freq / (1960.f + nyquist_freq) - 0.53f;
                    f = 1960.f * (bark_scaling * y + 0.53f) / (26.28f - bark_scaling * y);
                } else {
                    f = y * nyquist_freq;
                }
                s = Glib::ustring::compose("<tt>Frequency: %1 Hz</tt>", std::round(f));
            } else {
                float a = 20.f * std::log10(2.f * std::abs(y - 0.5f) / z.VerticalZoom());
                s = Glib::ustring::sprintf("<tt>Amplitude: %.1f dBFS</tt>", a);
            }
        }
    }

    if (s != str_frequency) {
        str_frequency = s;
        status_frequency.set_markup(str_frequency);
    }
}

void Gui::UpdateTitle() {
    Glib::ustring s = state->SelectedTrack() ? state->GetSelectedTrack().short_name : "wavey";
    if (s != str_title) {
        str_title = s;
        set_title(str_title);
    }
}
