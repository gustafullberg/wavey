#ifndef SPECTRUM_STATE_HPP_
#define SPECTRUM_STATE_HPP_

#include <complex.h>
#include <list>
#include <string>
#include <vector>
#include "state.hpp"

struct Spectrum {
    std::string name;
    std::future<std::vector<float>> future_spectrum;
    std::optional<std::vector<float>> spectrum;
    std::vector<float> frequencies;
    float color[3];
    float gain = 1.0f;
    bool visible = true;
};

class SpectrumState {
   public:
    SpectrumState() = default;

    void Add(const Track& track, float begin, float end, int channel);
    void Remove(std::list<Spectrum>::iterator it) { spectrums_.erase(it); }

    std::list<Spectrum>& spectrums() { return spectrums_; }

   private:
    std::list<Spectrum> spectrums_;
};

#endif  // SPECTRUM_STATE_HPP_
