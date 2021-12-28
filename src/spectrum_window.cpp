#include "spectrum_window.hpp"

#include <iostream>

#include "spectrum_state.hpp"
#include "state.hpp"

SpectrumWindow::SpectrumWindow(SpectrumState* state) : state_(state) {}

void SpectrumWindow::AddSpectrumFromTrack(const Track& t, float begin, float end, int channel) {
    std::cerr << "SpectrumWindow: Got Spectrum add channel " << channel << std::endl;
    state_->Add(t, begin, end, channel);
}
