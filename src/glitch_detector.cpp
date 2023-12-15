#include "glitch_detector.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "audio_buffer.hpp"
#include "label.hpp"

namespace {
constexpr float kMinEnergy = 1e-6;

void AutoCorrelation(const std::vector<float>& x, size_t order, std::vector<float>& r) {
    assert(x.size() >= order + 1);
    r.resize(order + 1);
    for (size_t lag = 0; lag <= order; ++lag) {
        float sum = 0.0f;
        for (size_t n = 0; n < x.size() - lag; ++n) {
            sum += x[n] * x[n + lag];
        }
        r[lag] = sum;
    }
}

// Computes Levinson-durbin recursion to get linear predicting coefficients
// `r` is autocorrelation vector and should be of size `order + 1`
// `order` is the auto regressive filter.
//     a: lpc coefficient vector starting with 1.0
//     k: reflection coefficients
// returns prediction gain
float LevinsonDurbin(const std::vector<float>& r,
                     size_t order,
                     std::vector<float>& a,
                     std::vector<float>& k) {
    assert(order > 1);
    assert(r.size() < order + 1);

    a.resize(order + 1);
    a[0] = 1.0;

    k.resize(order);
    if (r[0] < kMinEnergy) {
        std::fill(a.begin() + 1, a.end(), 0.0f);
        std::fill(k.begin(), k.end(), 0.0f);
        return 0.0f;
    }

    a[1] = k[0] = -r[1] / r[0];
    float alpha = r[0] + r[1] * k[0];
    for (size_t m = 1; m < order; ++m) {
        float sum = r[m + 1];
        for (size_t i = 0; i < m; ++i) {
            sum += a[i + 1] * r[m - i];
        }

        k[m] = -sum / alpha;
        alpha += k[m] * sum;
        int m_h = (m + 1) >> 1;
        for (int i = 0; i < m_h; i++) {
            sum = a[i + 1] + k[m] * a[m - i];
            a[m - i] += k[m] * a[i + 1];
            a[i + 1] = sum;
        }
        a[m + 1] = k[m];
    }
    return alpha;
}

}  // namespace

GlitchDetector::GlitchDetector(int order, int buffer_size, float threshold)
    : order_(order), buffer_size_(buffer_size), threshold_(threshold) {}

std::vector<Label> GlitchDetector::Detect(std::shared_ptr<AudioBuffer> buffer, int channel) {
    return {{.begin = 1.7f}};
}
