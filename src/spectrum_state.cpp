#include "spectrum_state.hpp"

#include <memory>

void SpectrumState::Add(const Track& track, float begin, float end, int channel){
  if(track.audio_buffer == nullptr)
    return;
  Spectrum s;
  std::shared_ptr<AudioBuffer> audio = track.audio_buffer;
  s.future_spectrum_ = std::async([audio](){return std::make_unique<std::vector<std::complex<float>>>();});
}
