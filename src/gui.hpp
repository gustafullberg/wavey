#ifndef GUI_HPP
#define GUI_HPP

#include <gtkmm.h>
#include "renderer.hpp"

class State;

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
    void SaveSelectionTo();
    void StartTimeUpdate();
    float TimelineHeight();
    void UpdateWidgets();
    bool UpdateTime();
    void UpdateZoom();
    void UpdatePointer();
    void UpdateTitle();
    void UpdateCurrentWorkingDirectory(std::string_view filename);
    void OnDroppedFiles(const Glib::RefPtr<Gdk::DragContext>& context,
                        int x,
                        int y,
                        const Gtk::SelectionData& selection_data,
                        guint info,
                        guint time);
    void OnTrackChanged(int watch_id);
    void OnActionAutoReload();
    void OnActionReload();
    void OnActionFollow();

    std::unique_ptr<Renderer> renderer;
    Gtk::HeaderBar headerbar;
    Gtk::Button open;
    Gtk::MenuButton menu_button;
    Glib::RefPtr<Gio::SimpleActionGroup> action_group;
    Glib::RefPtr<Gio::SimpleAction> action_autoreload;
    Glib::RefPtr<Gio::SimpleAction> action_reload;
    Glib::RefPtr<Gio::SimpleAction> action_follow;
    Glib::RefPtr<Gio::Menu> menu;
    Gtk::VBox box;
    Gtk::GLArea glarea;
    Gtk::Scrollbar scrollbar;
    Gtk::Grid grid_bottom;
    Gtk::Label status_time;
    Gtk::Label status_pointer;
    Gtk::Label label_selection;
    Gtk::Label label_pointer;
    int win_width = 0;
    int win_height = 0;
    float mouse_x = 0;
    float mouse_y = 0;
    bool mouse_down = false;
    bool view_spectrogram = false;
    bool view_bark_scale = false;
    bool follow_playback = true;
    State* state = nullptr;
    Glib::ustring str_title;
    Glib::ustring str_time;
    Glib::ustring str_pointer;

    // Last opened file.
    std::string current_working_directory;
};

#endif
