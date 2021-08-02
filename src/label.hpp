#ifndef LABEL_HPP
#define LABEL_HPP

#include <memory>
#include <string>

class Label {
   public:
    virtual ~Label() = default;
    virtual bool HasImageData() const = 0;
    virtual const unsigned char* ImageData() const = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;
    static std::unique_ptr<Label> CreateTrackLabel(const std::string& path,
                                                   const std::string& name,
                                                   int num_channels,
                                                   int samplerate);
    static std::unique_ptr<Label> CreateChannelLabel(const std::string& path,
                                                     const std::string& name,
                                                     int channel,
                                                     int num_channels,
                                                     int samplerate);
    static std::unique_ptr<Label> CreateTimeLabel(const std::string time);
};

#endif
