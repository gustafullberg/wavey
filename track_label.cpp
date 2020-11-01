#include "track_label.hpp"
#include <gtkmm.h>

class TrackLabelImpl : public TrackLabel {
   public:
    TrackLabelImpl(std::string name, int num_channels, int samplerate);
    virtual ~TrackLabelImpl() = default;
    virtual bool HasImageData() const;
    virtual const unsigned char* ImageData() const;
    virtual int Width() const;
    virtual int Height() const;

   private:
    bool DamageEvent(GdkEventExpose* event);
    Gtk::OffscreenWindow window;
    Gtk::Label label;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
};

TrackLabelImpl::TrackLabelImpl(std::string name, int num_channels, int samplerate) {
    std::string channels;
    if (num_channels == 1) {
        channels = "mono";
    } else if (num_channels == 2) {
        channels = "stereo";
    } else {
        channels = std::to_string(num_channels) + " channels";
    }
    window.set_opacity(0);
    window.override_color(Gdk::RGBA("white"));
    window.override_background_color(Gdk::RGBA("black"));
    label.set_margin_top(5);
    label.set_margin_left(5);
    label.set_markup("<b>" + name + "</b> - " + channels + " - " + std::to_string(samplerate) +
                     " Hz");
    window.add(label);
    window.signal_damage_event().connect(sigc::mem_fun(*this, &TrackLabelImpl::DamageEvent));
    window.show_all();
}

bool TrackLabelImpl::DamageEvent(GdkEventExpose* event) {
    pixbuf = window.get_pixbuf();
    return true;
}

bool TrackLabelImpl::HasImageData() const {
    return pixbuf ? true : false;
}

const unsigned char* TrackLabelImpl::ImageData() const {
    return pixbuf->get_pixels();
}

int TrackLabelImpl::Width() const {
    return pixbuf->get_width();
}

int TrackLabelImpl::Height() const {
    return pixbuf->get_height();
}

std::unique_ptr<TrackLabel> TrackLabel::Create(std::string name, int num_channels, int samplerate) {
    return std::make_unique<TrackLabelImpl>(name, num_channels, samplerate);
}
