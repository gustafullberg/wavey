#include "gui.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>

Gui::Gui(State* state) : state(state) {
    signal_key_press_event().connect(sigc::mem_fun(*this, &Gui::KeyPress), false);
    add_events(Gdk::KEY_PRESS_MASK);

    set_default_size(640, 480);
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

    statusbar.set_margin_top(0);
    statusbar.set_margin_bottom(0);
    box.pack_start(statusbar, false, true);
    show_all();
    UpdateStatus();
}

void Gui::Realize() {
    glarea.make_current();
    wave_shader.Init();
    spectrogram_shader.Init();
    prim_renderer.Init();

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Gui::Unrealize() {
    state->DeleteGpuBuffers();
    wave_shader.Terminate();
    spectrogram_shader.Terminate();
    prim_renderer.Terminate();
}

bool Gui::Render(const Glib::RefPtr<Gdk::GLContext> context) {
    state->UpdateGpuBuffers();

    glClear(GL_COLOR_BUFFER_BIT);

    float play_time;
    bool playing = state->Playing(&play_time);

    const ZoomWindow& z = state->zoom_window;
    glm::mat4 mvp = glm::ortho(z.Left(), z.Right(), z.Bottom(), z.Top(), -1.f, 1.f);

    for (int i = 0; i < static_cast<int>(state->tracks.size()); i++) {
        const Track& t = state->tracks[i];
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
            mvp_channel = glm::scale(mvp_channel, glm::vec3(1.f, -0.45f / num_channels, 1.f));

            glm::vec4 color_line(.5f, .9f, .5f, .1f);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, -1.f), glm::vec2(length, -1.f),
                                   color_line);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 1.f), glm::vec2(length, 1.f),
                                   color_line);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 0.f), glm::vec2(length, 0.f),
                                   color_line);
            if (view_spectrogram) {
                spectrogram_shader.Draw(mvp_channel);
                t.gpu_spectrogram->Draw(c);
            } else {
                wave_shader.Draw(mvp_channel, samplerate);
                t.gpu_waveform->Draw(c);
            }
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
        glarea.queue_render();
    }

    return true;
}

void Gui::Resize(int width, int height) {
    win_width = width;
    win_height = height;
    glarea.queue_render();
}

bool Gui::KeyPress(GdkEventKey* key_event) {
    bool ctrl = (key_event->state & Gtk::AccelGroup::get_default_mod_mask()) == Gdk::CONTROL_MASK;

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

    // Pan width arrow keys.
    if (key_event->keyval == GDK_KEY_Left) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() - adjustment->get_step_increment());
    } else if (key_event->keyval == GDK_KEY_Right) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() + adjustment->get_step_increment());
    }

    // Toggle spectrogram view.
    if (key_event->keyval == GDK_KEY_s) {
        view_spectrogram = !view_spectrogram;
    }

    // Start/stop playback.
    if (key_event->keyval == GDK_KEY_space) {
        state->TogglePlayback();
        glarea.queue_render();
    }

    glarea.queue_render();
    UpdateStatus();
    return true;
}

bool Gui::ButtonPress(GdkEventButton* button_event) {
    const float scale = get_scale_factor();
    const float x = button_event->x * scale / win_width;

    if (button_event->button == 1) {
        if (button_event->type == GDK_BUTTON_PRESS) {
            const float time = state->zoom_window.GetTime(x);
            state->SetCursor(time);
            mouse_down = true;
        } else if (button_event->type == GDK_2BUTTON_PRESS) {
            if (state->SelectedTrack()) {
                state->SetCursor(0.f);
                state->SetSelection(state->tracks[*state->SelectedTrack()].audio_buffer->Length());
            }
        }
        glarea.queue_render();
        UpdateStatus();
    }
    return true;
}

bool Gui::ButtonRelease(GdkEventButton* button_event) {
    if (button_event->button == 1) {
        if (button_event->type == GDK_BUTTON_RELEASE) {
            state->FixSelection();
            glarea.queue_render();
        }
        mouse_down = false;
    }
    return true;
}

bool Gui::PointerMove(GdkEventMotion* motion_event) {
    const float scale = get_scale_factor();
    const float x =
        std::min(std::max(static_cast<float>(motion_event->x) * scale / win_width, 0.f), 1.f);
    const float y =
        std::min(std::max(static_cast<float>(motion_event->y) * scale / win_height, 0.f), 1.f);

    if (mouse_down) {
        const float time = state->zoom_window.GetTime(x);
        state->SetSelection(time);
        glarea.queue_render();
        UpdateStatus();
    }

    bool changed = state->SetSelectedTrack(state->zoom_window.GetTrack(y));
    if (changed) {
        glarea.queue_render();
        UpdateStatus();
    }

    return true;
}

bool Gui::ScrollWheel(GdkEventScroll* scroll_event) {
    const float scale = get_scale_factor();
    const float x = scroll_event->x * scale / win_width;

    GdkScrollDirection dir = scroll_event->direction;
    if (dir == GDK_SCROLL_UP) {
        state->zoom_window.ZoomIn(x);
    } else if (dir == GDK_SCROLL_DOWN) {
        state->zoom_window.ZoomOut(x);
    } else if (dir == GDK_SCROLL_LEFT) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() - adjustment->get_step_increment());
    } else if (dir == GDK_SCROLL_RIGHT) {
        auto adjustment = scrollbar.get_adjustment();
        scrollbar.set_value(scrollbar.get_value() + adjustment->get_step_increment());
    }

    glarea.queue_render();
    UpdateStatus();
    return true;
}

void Gui::Scrolling() {
    auto adjustment = scrollbar.get_adjustment();
    state->zoom_window.PanTo(adjustment->get_value());
    glarea.queue_render();
}

void Gui::UpdateStatus() {
    if (state->SelectedTrack()) {
        set_title(state->tracks[*state->SelectedTrack()].path);
    } else {
        set_title("wavey");
    }

    const ZoomWindow& z = state->zoom_window;
    const float page_size = z.Right() - z.Left();
    auto adjustment = scrollbar.get_adjustment();
    adjustment->set_lower(0);
    adjustment->set_upper(z.MaxX());
    adjustment->set_page_size(page_size);
    adjustment->set_step_increment(0.1f * page_size);
    adjustment->set_page_increment(page_size);
    adjustment->set_value(z.Left());

    Glib::ustring s;

    if (state->Selection()) {
        float s1 = state->Cursor();
        float s2 = *state->Selection();
        if (s2 < s1)
            std::swap(s1, s2);
        s += Glib::ustring::compose("Selection %1 - %2",
                                    Glib::ustring::format(std::fixed, std::setprecision(3), s1),
                                    Glib::ustring::format(std::fixed, std::setprecision(3), s2));
    } else {
        float s1 = state->Cursor();
        s += Glib::ustring::compose("Cursor %1",
                                    Glib::ustring::format(std::fixed, std::setprecision(3), s1));
    }
    statusbar.push(s, 0);
}
