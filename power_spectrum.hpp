#ifndef POSER_SPECTRUM_HPP
#define POWER_SPECTRUM_HPP

#include <array>
#include <vector>

constexpr int kInputSize = 1024;
constexpr int kInputAdvance = 1024 / 2;
constexpr int kOutputSize = kInputSize / 2 + 1;

class PowerSpectrum {
   public:
    PowerSpectrum(const float* samples, int num_channels, int num_frames);

   private:
    std::vector<std::vector<std::array<float, kOutputSize>>> power_spectra;
};

#endif
