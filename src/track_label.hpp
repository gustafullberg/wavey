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
    static std::unique_ptr<TrackLabel> CreateTrackLabel(const std::string& path,
                                                        const std::string& name,
                                                        int num_channels,
                                                        int samplerate);
    static std::unique_ptr<TrackLabel> CreateChannelLabel(const std::string& path,
                                                          const std::string& name,
                                                          int channel,
                                                          int num_channels,
                                                          int samplerate);
    static std::unique_ptr<TrackLabel> CreateTimeLabel(const std::string time);
};

#endif
