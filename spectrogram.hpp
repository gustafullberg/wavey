#ifndef SPECTROGRAM_HPP
#define SPECTROGRAM_HPP

#include <array>
#include <vector>

constexpr int kInputSize = 1024;
constexpr int kInputAdvance = 1024 / 2;
constexpr int kOutputSize = kInputSize / 2 + 1;

class Spectrogram {
   public:
    Spectrogram(const float* samples, int num_channels, int num_frames);

   private:
    std::vector<std::vector<std::array<float, kOutputSize>>> power_spectra;
};

#endif
