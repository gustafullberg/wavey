#include "label.hpp"
#include <gtkmm.h>

class LabelImpl : public Label {
   public:
    LabelImpl(const std::string& path, const std::string& name, int num_channels, int samplerate);
    LabelImpl(const std::string& path,
              const std::string& name,
              int channel,
              int num_channels,
              int samplerate);
    LabelImpl(const std::string& s);
    virtual ~LabelImpl() = default;
    void Init(const std::string& s);
    virtual bool HasImageData() const;
    virtual const unsigned char* ImageData() const;
    virtual int Width() const;
    virtual int Height() const;
    void RemoveMargin();

   private:
    bool DamageEvent(GdkEventExpose* event);
    Gtk::OffscreenWindow window;
    Gtk::Label label;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
};

LabelImpl::LabelImpl(const std::string& path,
                     const std::string& name,
                     int num_channels,
                     int samplerate) {
    std::string channels;
    if (num_channels == 1) {
        channels = "mono";
    } else if (num_channels == 2) {
        channels = "stereo";
    } else {
        channels = std::to_string(num_channels) + " channels";
    }

    std::string s;
    if (num_channels) {
        s = "<b>" + name + "</b> - " + channels + " - " + std::to_string(samplerate) + " Hz";
    } else {
        s = "<b>Error:</b> Failed to load " + path;
    }
    Init(s);
}

LabelImpl::LabelImpl(const std::string& path,
                     const std::string& name,
                     int channel,
                     int num_channels,
                     int samplerate) {
    std::string channels =
        "channel " + std::to_string(channel) + "/" + std::to_string(num_channels);

    std::string s;
    if (num_channels) {
        s = "<b>" + name + "</b> - " + channels + " - " + std::to_string(samplerate) + " Hz";
    } else {
        s = "<b>Error:</b> Failed to load " + path;
    }
    Init(s);
}

LabelImpl::LabelImpl(const std::string& s) {
    Init(s);
}

void LabelImpl::Init(const std::string& s) {
    window.override_color(Gdk::RGBA("white"));
    window.override_background_color(Gdk::RGBA("black"));
    label.override_background_color(Gdk::RGBA("black"));
    label.set_margin_start(5);
    label.set_margin_top(5);
    label.set_markup(s);
    window.add(label);
    window.signal_damage_event().connect(sigc::mem_fun(*this, &LabelImpl::DamageEvent));
    window.show_all();
}

void LabelImpl::RemoveMargin() {
    label.set_margin_start(0);
    label.set_margin_top(0);
}

bool LabelImpl::DamageEvent(GdkEventExpose* event) {
    pixbuf = window.get_pixbuf();
    return true;
}

bool LabelImpl::HasImageData() const {
    return pixbuf ? true : false;
}

const unsigned char* LabelImpl::ImageData() const {
    return pixbuf->get_pixels();
}

int LabelImpl::Width() const {
    return pixbuf->get_width();
}

int LabelImpl::Height() const {
    return pixbuf->get_height();
}

std::unique_ptr<Label> Label::CreateTrackLabel(const std::string& path,
                                               const std::string& name,
                                               int num_channels,
                                               int samplerate) {
    return std::make_unique<LabelImpl>(path, name, num_channels, samplerate);
}

std::unique_ptr<Label> Label::CreateChannelLabel(const std::string& path,
                                                 const std::string& name,
                                                 int channel,
                                                 int num_channels,
                                                 int samplerate) {
    return std::make_unique<LabelImpl>(path, name, channel, num_channels, samplerate);
}

std::unique_ptr<Label> Label::CreateTimeLabel(const std::string time) {
    LabelImpl* label = new LabelImpl("<small>" + time + "</small>");
    label->RemoveMargin();
    return std::unique_ptr<Label>(label);
}
