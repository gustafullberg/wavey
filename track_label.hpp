#ifndef TRACK_LABEL_HPP
#define TRACK_LABEL_HPP

#include <memory>
#include <string>

class TrackLabel {
   public:
    virtual ~TrackLabel() = default;
    virtual bool HasImageData() const = 0;
    virtual const unsigned char* ImageData() const = 0;
    virtual int Width() const = 0;
    virtual int Height() const = 0;
    static std::unique_ptr<TrackLabel> Create(const std::string& path,
                                              const std::string& name,
                                              int num_channels,
                                              int samplerate);
};

#endif
