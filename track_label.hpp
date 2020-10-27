#ifndef TRACK_LABEL_HPP
#define TRACK_LABEL_HPP

#include <gtkmm.h>

class TrackLabel {
   public:
    TrackLabel(std::string name);
    bool HasImageData() const;
    const unsigned char* ImageData() const;
    int Width() const;
    int Height() const;

   private:
    bool DamageEvent(GdkEventExpose* event);
    Gtk::OffscreenWindow window;
    Gtk::Label label;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
};

#endif
