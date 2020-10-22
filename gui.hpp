#ifndef GUI_HPP
#define GUI_HPP

#include <gtkmm.h>
#include "primitive_renderer.hpp"
#include "spectrogram_shader.hpp"
#include "state.hpp"
#include "wave_shader.hpp"

class Gui : public Gtk::Window {
   public:
    Gui(State* state);

   private:
    void Realize();
    void Unrealize();
    bool Render(const Glib::RefPtr<Gdk::GLContext> context);
    void Resize(int width, int height);
    bool KeyPress(GdkEventKey* key_event);
    bool ButtonPress(GdkEventButton* button_event);
    bool ButtonRelease(GdkEventButton* button_event);
    bool PointerMove(GdkEventMotion* motion_event);
    bool ScrollWheel(GdkEventScroll* scroll_event);
    void Scrolling();
    void StartTimeUpdate();
    bool UpdateTime();
    void UpdateZoom();
    void UpdateSelection();
    void UpdateTitle();
    Gtk::VBox box;
    Gtk::GLArea glarea;
    Gtk::Scrollbar scrollbar;
    Gtk::Grid grid_top;
    Gtk::Grid grid_bottom;
    Gtk::Label status_time;
    Gtk::Label status_selection;
    Gtk::Label status_view_start;
    Gtk::Label status_view_end;
    WaveShader wave_shader;
    SpectrogramShader spectrogram_shader;
    PrimitiveRenderer prim_renderer;
    int win_width = 0;
    int win_height = 0;
    bool mouse_down = false;
    bool view_spectrogram = false;
    State* state = nullptr;
};

#endif
