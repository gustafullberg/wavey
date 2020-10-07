#ifndef GUI_HPP
#define GUI_HPP

#include <gtkmm.h>
#include "primitive_renderer.hpp"
#include "state.hpp"
#include "wave_shader.hpp"

class Gui : public Gtk::Window {
   public:
    Gui(State* state);
    void Realize();
    void Unrealize();
    bool Render(const Glib::RefPtr<Gdk::GLContext> context);
    void Resize(int width, int height);
    bool KeyPress(GdkEventKey* key_event);
    bool KeyRelease(GdkEventKey* key_event);
    bool ButtonPress(GdkEventButton* button_event);
    bool ButtonRelease(GdkEventButton* button_event);
    bool PointerMove(GdkEventMotion* motion_event);
    bool Scroll(GdkEventScroll* scroll_event);

   private:
    Gtk::VBox box;
    Gtk::GLArea glarea;
    // Gtk::Button btn;
    WaveShader wave_shader;
    PrimitiveRenderer prim_renderer;
    int win_width = 0;
    int win_height = 0;
    bool mouse_down = false;
    State* state = nullptr;
};

#endif
