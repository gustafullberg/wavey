#include "track_label.hpp"

TrackLabel::TrackLabel(std::string name) {
    window.set_opacity(0);
    window.override_color(Gdk::RGBA("white"));
    window.override_background_color(Gdk::RGBA("black"));
    label.set_margin_top(5);
    label.set_margin_left(5);
    label.set_markup("<b>" + name + "</b>");
    window.add(label);
    window.signal_damage_event().connect(sigc::mem_fun(*this, &TrackLabel::DamageEvent));
    window.show_all();
}

bool TrackLabel::DamageEvent(GdkEventExpose* event) {
    pixbuf = window.get_pixbuf();
    return true;
}

bool TrackLabel::HasImageData() const {
    return pixbuf ? true : false;
}

const unsigned char* TrackLabel::ImageData() const {
    return pixbuf->get_pixels();
}
int TrackLabel::Width() const {
    return pixbuf->get_width();
}

int TrackLabel::Height() const {
    return pixbuf->get_height();
}
