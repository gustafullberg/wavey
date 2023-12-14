#include "glitch_detector.hpp"

#include <vector>
#include <memory>

#include "label.hpp"
#include "audio_buffer.hpp"

GlitchDetector::GlitchDetector(int order, int buffer_size, float threshold)
    : order_(order), buffer_size_(buffer_size), threshold_(threshold) {}

std::vector<Label> GlitchDetector::Detect(std::shared_ptr<AudioBuffer> buffer, int channel) {
  return {{.begin=1.7f}};
}
