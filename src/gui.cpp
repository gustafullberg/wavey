#include "gui.hpp"
#include "state.hpp"

namespace {
Glib::ustring FormatTime(float t, bool show_minutes = true) {
    float minutes = std::floor(t / 60.f);
    float seconds = t - 60.f * minutes;
    if (show_minutes || minutes > 0.f) {
        return Glib::ustring::sprintf("%02.f:%06.03f", minutes, seconds);
    } else {
        return Glib::ustring::sprintf("%.03f", seconds);
    }
}
}  // namespace

Gui::Gui(State* state) : state(state) {
    // CSS rules
    auto css = Gtk::CssProvider::create();
    std::string rules;
    rules += ".offscreenbackground { background-color: black; color: white; }";
    rules += ".offscreenmargin { margin-left: 5px; margin-top: 5px; }";
    css->load_from_data(rules);
    auto screen = Gdk::Screen::get_default();
    auto ctx = get_style_context();
    ctx->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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
    grid_bottom.attach(label_selection, 0, 0, 1, 1);
    grid_bottom.attach(status_time, 1, 0, 1, 1);
    grid_bottom.attach(label_pointer, 0, 1, 1, 1);
    grid_bottom.attach(status_pointer, 1, 1, 1, 1);
    status_time.set_xalign(0);
    status_time.set_margin_start(5);
    status_time.set_margin_end(5);
    label_selection.set_xalign(0);
    label_selection.set_margin_start(5);
    label_selection.set_margin_end(5);
    label_selection.set_markup("<small>Selection</small>");
    label_pointer.set_xalign(0);
    label_pointer.set_margin_start(5);
    label_pointer.set_margin_end(5);
    label_pointer.set_markup("<small>Pointer</small>");
    status_pointer.set_xalign(0);
    status_pointer.set_margin_start(5);
    status_pointer.set_margin_end(5);

    show_all();
    UpdateWidgets();

    std::vector<Gtk::TargetEntry> list_drop_targets = {
        Gtk::TargetEntry("text/uri-list"),
    };
    drag_dest_set(list_drop_targets, Gtk::DEST_DEFAULT_MOTION | Gtk::DEST_DEFAULT_DROP,
                  Gdk::ACTION_COPY | Gdk::ACTION_MOVE);
    signal_drag_data_received().connect(sigc::mem_fun(*this, &Gui::OnDroppedFiles));
}

void Gui::OnDroppedFiles(const Glib::RefPtr<Gdk::DragContext>& context,
                         int x,
                         int y,
                         const Gtk::SelectionData& selection_data,
                         guint info,
                         guint time) {
    if ((selection_data.get_length() >= 0) && (selection_data.get_format() == /*string*/ 8)) {
        for (const auto& file_uri : selection_data.get_uris()) {
            Glib::ustring path = Glib::filename_from_uri(file_uri);
            state->LoadFile(path);
        }
    }

    context->drag_finish(true, false, time);
}

void Gui::Realize() {
    glarea.make_current();
    renderer.reset(Renderer::Create());
}

void Gui::Unrealize() {
    state->UnloadFiles();
    renderer.reset();
}

bool Gui::Render(const Glib::RefPtr<Gdk::GLContext> context) {
    float play_time = 0.f;
    bool playing = state->Playing(&play_time);

    renderer->Draw(state, win_width, win_height, get_scale_factor(), view_spectrogram,
                   view_bark_scale, playing, play_time);

    bool view_reset;
    bool resources_to_load = state->CreateResources(&view_reset);
    if (view_reset) {
        UpdateWidgets();
    }

    // Redraw continuously until all resources are loaded and if playing audio.
    if (resources_to_load || playing) {
        queue_draw();
    }
    return true;
}

float Gui::TimelineHeight() {
    return renderer ? renderer->TimelineHeight() : 0;
}

void Gui::Resize(int width, int height) {
    win_width = width;
    win_height = height;
    queue_draw();
}

bool Gui::KeyPress(GdkEventKey* key_event) {
    bool ctrl = (key_event->state & Gtk::AccelGroup::get_default_mod_mask()) & Gdk::CONTROL_MASK;
    bool shift = (key_event->state & Gtk::AccelGroup::get_default_mod_mask()) & Gdk::SHIFT_MASK;

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

    // Follow playback
    if (key_event->keyval == GDK_KEY_f && !ctrl) {
        follow_playback = !follow_playback;
    }

    // Zoom to selection.
    if (key_event->keyval == GDK_KEY_e && ctrl && state->Selection()) {
        state->zoom_window.ZoomRange(state->Cursor(), *state->Selection());
    }

    // Zoom toggle one/all tracks.
    if (key_event->keyval == GDK_KEY_z) {
        state->ToggleViewSingleTrack();
    }

    // Zoom toggle one/all channels.
    if (key_event->keyval == GDK_KEY_Z) {
        state->ToggleViewSingleChannel(mouse_y);
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
        if (shift) {
            state->MoveTrackUp();
        } else {
            state->ScrollUp();
        }
    } else if (key_event->keyval == GDK_KEY_Down) {
        if (shift) {
            state->MoveTrackDown();
        } else {
            state->ScrollDown();
        }
    }

    // Save selection.
    if (key_event->keyval == GDK_KEY_s && ctrl) {
        SaveSelectionTo();
    }

    // Toggle spectrogram view.
    if (key_event->keyval == GDK_KEY_s && !ctrl) {
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
        state->SetLooping(shift);
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
        state->UnloadSelectedTrack();
    }

    // Close all tracks.
    if (key_event->keyval == GDK_KEY_W && ctrl) {
        state->UnloadFiles();
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
                state->SetSelection(state->GetSelectedTrack().audio_buffer->Duration());
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
    const float timeline_height = TimelineHeight();
    const float view_height = win_height - timeline_height;
    const float scale = get_scale_factor();
    mouse_x = std::min(std::max(static_cast<float>(motion_event->x) * scale / win_width, 0.f), 1.f);
    mouse_y = std::min(
        std::max((static_cast<float>(motion_event->y) - timeline_height) * scale / view_height,
                 0.f),
        1.f);

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

void Gui::UpdateCurrentWorkingDirectory(std::string_view filename) {
    size_t last_sep = filename.find_last_of('/');
    if (last_sep != std::string::npos) {
        current_working_directory = filename.substr(0, last_sep);
    }
}

void Gui::ChooseFiles() {
    Gtk::FileChooserDialog dialog("Open", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    dialog.set_select_multiple();
    if (current_working_directory.size()) {
        dialog.set_current_folder(current_working_directory);
    }
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        std::vector<std::string> files = dialog.get_filenames();
        for (std::string file : files) {
            state->LoadFile(file);
            UpdateCurrentWorkingDirectory(file);
        }
    }
}

void Gui::SaveSelectionTo() {
    Gtk::FileChooserDialog dialog("Save Selection", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    if (!current_working_directory.empty()) {
        dialog.set_current_folder(current_working_directory);
    }
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK && state->Selection()) {
        const float start = std::min(state->Cursor(), *state->Selection());
        const float end = std::max(state->Cursor(), *state->Selection());
        std::shared_ptr<AudioBuffer> audio_buffer = state->GetSelectedTrack().audio_buffer;
        if (start < end && start < audio_buffer->Duration() && end < audio_buffer->Duration()) {
            std::string filename = dialog.get_filename();
            audio_buffer->SaveTo(start, end, filename);
            UpdateCurrentWorkingDirectory(filename);
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
    UpdatePointer();
    UpdateTitle();
}

bool Gui::UpdateTime() {
    float play_time;
    bool playing = state->Playing(&play_time);
    float time;
    if (playing) {
        time = play_time;
        if (follow_playback) {
            if (play_time > state->zoom_window.Right() || play_time < state->zoom_window.Left()) {
                state->zoom_window.PanTo(play_time);
            }
        }
    } else {
        time = state->Cursor();
    }

    Glib::ustring s;
    if (playing || !state->Selection()) {
        s = Glib::ustring::compose("<tt><b>%1</b></tt> <small>%2</small>", FormatTime(time),
                                   follow_playback && playing ? "[follow]" : "");
    } else {
        float s_start = time;
        float s_end = *state->Selection();
        if (s_end < s_start)
            std::swap(s_start, s_end);

        const float s_duration = s_end - s_start;
        const int samplerate = state->SelectedTrack() ? state->GetCurrentSamplerate() : 0;

        // Start time, end time and selection length.
        s = Glib::ustring::compose(
            "<tt><b>%1</b></tt> - <tt><b>%2</b>  </tt><small>Length </small><tt>%3</tt>",
            FormatTime(s_start), FormatTime(s_end), FormatTime(s_duration, false));

        // Duration in samples.
        s += Glib::ustring::sprintf("<tt>  %d</tt><small> samples</small>",
                                    static_cast<int>(s_duration * samplerate));

        // Duration in Hertz.
        if (s_duration > 1e-5f && s_duration < 10.f) {
            float frequency = 1.f / s_duration;
            s += Glib::ustring::sprintf("<tt>  %.03f</tt><small> Hz</small>", frequency);
        }
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
    adjustment->set_step_increment(std::max(0.1f * page_size, 0.001f));
    adjustment->set_page_increment(page_size);
    adjustment->set_value(z.Left());
}

void Gui::UpdatePointer() {
    Glib::ustring s;
    ZoomWindow& z = state->zoom_window;
    const float time = z.GetTime(mouse_x);
    s = Glib::ustring::compose("<tt><b>%1</b>  </tt>", FormatTime(time));

    if (state->tracks.size()) {
        ZoomWindow& z = state->zoom_window;
        int track_number = z.GetTrack(mouse_y);
        Track& t = state->GetTrack(track_number);
        float display_gain_db = 20.0f * std::log10(z.VerticalZoom());
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
                s += Glib::ustring::sprintf(
                    "<small>Frequency </small><tt>%.0f</tt><small> Hz</small><tt>  </tt>",
                    std::round(f));
            } else {
                float a = 20.f * std::log10(2.f * std::abs(y - 0.5f) / z.VerticalZoom());
                s += Glib::ustring::sprintf(
                    "<small>Amplitude </small><tt>%.1f</tt><small> dBFS</small><tt>  </tt>", a);
            }
            s += Glib::ustring::sprintf("<small>Gain </small><tt>%.1f</tt><small> dB</small>",
                                        display_gain_db);
        }
    }
    if (s != str_pointer) {
        str_pointer = s;
        status_pointer.set_markup(str_pointer);
    }
}

void Gui::UpdateTitle() {
    Glib::ustring s = state->SelectedTrack() ? state->GetSelectedTrack().short_name : "wavey";
    if (s != str_title) {
        str_title = s;
        set_title(str_title);
    }
}
