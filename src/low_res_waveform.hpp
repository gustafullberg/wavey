#ifndef LOW_RES_WAVEFORM_HPP
#define LOW_RES_WAVEFORM_HPP

#include "audio_buffer.hpp"

class LowResWaveform {
   public:
    LowResWaveform(const AudioBuffer& ab);
    ~LowResWaveform() = default;
    const std::vector<float>& GetBuffer() { return buffer; }

   private:
    std::vector<float> buffer;
};

#endif
