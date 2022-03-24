#ifndef SPECTRUM_STATE_HPP_
#define SPECTRUM_STATE_HPP_

#include <list>
#include <string>
#include <complex>
#include <vector>
#include "state.hpp"
#include <fftw3.h>


struct Spectrum {
  std::string name_;
  std::future<std::vector<float>> future_spectrum_;
  float gain_ = 1.0f;
};

class SpectrumState {
 public:
  SpectrumState() = default;

  void Add(const Track& track, float begin, float end, int channel);

 private:
  std::list<std::unique_ptr<Spectrum>> spectrums_;
};


#endif  // SPECTRUM_STATE_HPP_
