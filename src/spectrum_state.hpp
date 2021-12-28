#ifndef SPECTRUM_STATE_HPP_
#define SPECTRUM_STATE_HPP_

#include <list>
#include <string>
#include <complex>
#include <vector>
#include "state.hpp"

class Spectrum {
  std::string name_;
  std::future<std::unique_ptr<std::vector<std::complex<float>>>> future_spectrum_;
  float gain_ = 1.0f;
};

class SpectrumState {
 public:
  SpectrumState() = default;

  void Add(const Track& track, float begin, float end, int channel);

 private:
  std::list<Spectrum> spectrums_;
};


#endif  // SPECTRUM_STATE_HPP_
