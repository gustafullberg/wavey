#ifndef GLITCH_DETECTOR_H_
#define GLITCH_DETECTOR_H_

#include "audio_buffer.hpp"
#include "label.hpp"

#include <memory>
#include <vector>

class GlitchDetector {
   public:
    GlitchDetector(int order = 12, int buffer_size = 480, float threshold = 0.001);

    std::vector<Label> Detect(std::shared_ptr<AudioBuffer> buffer, int channel);

   private:
    int order_;
    int buffer_size_;
    float threshold_;
};

#endif
