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
    std::optional<std::vector<float>> spectrum() {
        if (future_spectrum.valid()) {
            if (future_spectrum.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                spectrum_ = future_spectrum.get();
            }
        }
        return spectrum_;
    }
    std::vector<float> frequencies;
    float color[3];
    float gain = 1.0f;
    bool visible = true;

   private:
    std::optional<std::vector<float>> spectrum_;
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
