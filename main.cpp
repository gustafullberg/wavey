#include <gtkmm.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <iostream>
#include "audio_buffer.hpp"
#include "audio_system.hpp"
#include "gl_waveform.hpp"
#include "quad_renderer.hpp"
#include "state.hpp"
#include "wave_shader.hpp"

class MainWindow : public Gtk::Window {
   public:
    MainWindow(State* state) : state(state) {
        set_title("Wavey");

        signal_key_press_event().connect(sigc::mem_fun(*this, &MainWindow::KeyPress));
        signal_key_release_event().connect(sigc::mem_fun(*this, &MainWindow::KeyRelease));
        add_events(Gdk::KEY_PRESS_MASK);
        add_events(Gdk::KEY_RELEASE_MASK);

        set_default_size(640, 480);
        add(box);

        glarea.set_required_version(3, 3);
        glarea.signal_realize().connect(sigc::mem_fun(*this, &MainWindow::Realize));
        glarea.signal_unrealize().connect(sigc::mem_fun(*this, &MainWindow::Unrealize));
        glarea.signal_render().connect(sigc::mem_fun(*this, &MainWindow::Render));
        glarea.signal_resize().connect(sigc::mem_fun(*this, &MainWindow::Resize));
        glarea.signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::ButtonPress));
        glarea.signal_button_release_event().connect(
            sigc::mem_fun(*this, &MainWindow::ButtonRelease));
        glarea.signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::PointerMove));
        glarea.signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::Scroll));
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

    void Realize() {
        glarea.make_current();
        wave_shader.Init();
        quad_renderer.Init();
    }

    void Unrealize() {
        wave_shader.Terminate();
        quad_renderer.Terminate();
    }

    bool Render(const Glib::RefPtr<Gdk::GLContext> context) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const ZoomWindow& z = state->zoom_window;
        for (int buffer_number = 0; buffer_number < state->tracks.size(); buffer_number++) {
            if (buffer_number == state->selected_track) {
                quad_renderer.Draw(0.f, z.GetY(buffer_number), 1.f,
                                   z.GetY(buffer_number + 1) - z.GetY(buffer_number));
                quad_renderer.Draw(0.f, 1.f, 0.5f, 0.2f);
            }

            const AudioBuffer& ab = *state->tracks[buffer_number].audio_buffer;

            if (!state->tracks[buffer_number].gpu_buffer) {
                state->tracks[buffer_number].gpu_buffer = std::make_unique<GLWaveform>(ab);
            }

            for (size_t c = 0; c < ab.NumChannels(); c++) {
                wave_shader.Draw(z.Left(), z.Right(), z.Top(), z.Bottom(), buffer_number, c,
                                 ab.NumChannels(), ab.Samplerate());
                state->tracks[buffer_number].gpu_buffer->Draw(c);
            }
        }
        if (state->selection_end >= 0.f) {
            quad_renderer.Draw(z.GetX(state->selection_start), 0.f,
                               z.GetX(state->selection_end) - z.GetX(state->selection_start), 1.f);
        }
        return true;
    }

    void Resize(int width, int height) {
        win_width = width;
        win_height = height;
    }

    bool KeyPress(GdkEventKey* key_event) { return true; }

    bool KeyRelease(GdkEventKey* key_event) {
        if (key_event->keyval == GDK_KEY_space) {
            state->TogglePlayback();
        }
        return true;
    }

    bool ButtonPress(GdkEventButton* button_event) {
        const float time = state->zoom_window.GetTime(button_event->x / win_width);
        state->selection_start = time;
        state->selection_end = -1.f;

        glarea.queue_render();
        mouse_down = true;
        return true;
    }

    bool ButtonRelease(GdkEventButton* button_event) {
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

    bool PointerMove(GdkEventMotion* motion_event) {
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

    bool Scroll(GdkEventScroll* scroll_event) {
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

    Gtk::VBox box;
    Gtk::GLArea glarea;
    // Gtk::Button btn;
    WaveShader wave_shader;
    QuadRenderer quad_renderer;
    int win_width = 0;
    int win_height = 0;
    bool mouse_down = false;
    State* state = nullptr;
};

int main(int argc, char** argv) {
    AudioSystem audio;
    State state(&audio);
    for (int i = 1; i < argc; i++) {
        state.LoadFile(argv[i]);
    }
    argc = 1;
    auto app = Gtk::Application::create(argc, argv, "test.wavelet");
    MainWindow win(&state);
    app->run(win);
    return 0;
}
