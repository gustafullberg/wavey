#include "gui.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Gui::Gui(State* state) : state(state) {
    set_title("Wavey");

    signal_key_press_event().connect(sigc::mem_fun(*this, &Gui::KeyPress));
    signal_key_release_event().connect(sigc::mem_fun(*this, &Gui::KeyRelease));
    add_events(Gdk::KEY_PRESS_MASK);
    add_events(Gdk::KEY_RELEASE_MASK);

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
    glarea.signal_scroll_event().connect(sigc::mem_fun(*this, &Gui::Scroll));
    glarea.add_events(Gdk::BUTTON_PRESS_MASK);
    glarea.add_events(Gdk::BUTTON_RELEASE_MASK);
    glarea.add_events(Gdk::POINTER_MOTION_MASK);
    glarea.add_events(Gdk::SCROLL_MASK);
    glarea.set_auto_render(false);
    box.pack_start(glarea);

    // btn.set_label("Test");
    // box.pack_start(btn, false, true);
    show_all();
}

void Gui::Realize() {
    glarea.make_current();
    wave_shader.Init();
    prim_renderer.Init();

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Gui::Unrealize() {
    state->DeleteGpuBuffers();
    wave_shader.Terminate();
    prim_renderer.Terminate();
}

bool Gui::Render(const Glib::RefPtr<Gdk::GLContext> context) {
    state->UpdateGpuBuffers();

    glClear(GL_COLOR_BUFFER_BIT);

    const ZoomWindow& z = state->zoom_window;
    glm::mat4 mvp = glm::ortho(z.Left(), z.Right(), z.Bottom(), z.Top(), -1.f, 1.f);

    for (int i = 0; i < static_cast<int>(state->tracks.size()); i++) {
        const Track& t = state->tracks[i];
        if (i == state->selected_track) {
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
            wave_shader.Draw(mvp_channel, samplerate);
            t.gpu_buffer->Draw(c);
        }
    }
    if (state->selection_end >= 0.f) {
        glm::vec4 color_selection(.5f, .9f, .5f, .1f);
        prim_renderer.DrawQuad(mvp, glm::vec2(state->selection_start, z.Bottom()),
                               glm::vec2(state->selection_end, z.Top()), color_selection);
    }
    return true;
}

void Gui::Resize(int width, int height) {
    win_width = width;
    win_height = height;
}

bool Gui::KeyPress(GdkEventKey* key_event) {
    return true;
}

bool Gui::KeyRelease(GdkEventKey* key_event) {
    if (key_event->keyval == GDK_KEY_space) {
        state->TogglePlayback();
    }
    return true;
}

bool Gui::ButtonPress(GdkEventButton* button_event) {
    const float time = state->zoom_window.GetTime(button_event->x / win_width);
    state->selection_start = time;
    state->selection_end = -1.f;

    glarea.queue_render();
    mouse_down = true;
    return true;
}

bool Gui::ButtonRelease(GdkEventButton* button_event) {
    const float time = state->zoom_window.GetTime(button_event->x / win_width);
    if (time > state->selection_start) {
        state->selection_end = time;
    } else if (time < state->selection_start) {
        state->selection_end = state->selection_start;
        state->selection_start = time;
    }

    glarea.queue_render();
    mouse_down = false;
    return true;
}

bool Gui::PointerMove(GdkEventMotion* motion_event) {
    if (mouse_down) {
        const float time = state->zoom_window.GetTime(motion_event->x / win_width);
        state->selection_end = time;
        glarea.queue_render();
    }

    int old_selected_track = state->selected_track;
    state->selected_track = state->zoom_window.GetTrack(motion_event->y / win_height);
    if (state->selected_track != old_selected_track) {
        glarea.queue_render();
    }

    return true;
}

bool Gui::Scroll(GdkEventScroll* scroll_event) {
    GdkScrollDirection dir = scroll_event->direction;
    float x = scroll_event->x / win_width;
    if (dir == GDK_SCROLL_UP) {
        state->zoom_window.ZoomIn(x);
    } else if (dir == GDK_SCROLL_DOWN) {
        state->zoom_window.ZoomOut(x);
    } else if (dir == GDK_SCROLL_LEFT) {
        state->zoom_window.PanLeft();
    } else if (dir == GDK_SCROLL_RIGHT) {
        state->zoom_window.PanRight();
    }

    glarea.queue_render();
    return true;
}
