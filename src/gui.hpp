#ifndef GUI_HPP
#define GUI_HPP

#include <gtkmm.h>
#include "label_renderer.hpp"
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
    void ChooseFiles();
    void StartTimeUpdate();
    void UpdateWidgets();
    bool UpdateTime();
    void UpdateZoom();
    void UpdateFrequency();
    void UpdateTitle();

    Gtk::HeaderBar headerbar;
    Gtk::Button open;
    Gtk::VBox box;
    Gtk::GLArea glarea;
    Gtk::Scrollbar scrollbar;
    Gtk::Grid grid_top;
    Gtk::Grid grid_bottom;
    Gtk::Label status_time;
    Gtk::Label status_frequency;
    Gtk::Label status_view_start;
    Gtk::Label status_view_end;
    Gtk::Label status_view_length;
    WaveShader wave_shader;
    SpectrogramShader spectrogram_shader;
    PrimitiveRenderer prim_renderer;
    LabelRenderer label_renderer;
    int win_width = 0;
    int win_height = 0;
    float mouse_x = 0;
    float mouse_y = 0;
    bool mouse_down = false;
    bool view_spectrogram = false;
    bool view_bark_scale = false;
    State* state = nullptr;
    Glib::ustring str_title;
    Glib::ustring str_time;
    Glib::ustring str_frequency;
    Glib::ustring str_view_start;
    Glib::ustring str_view_end;
    Glib::ustring str_view_length;

    // Last opened file.
    std::string open_dir;
};

#endif
