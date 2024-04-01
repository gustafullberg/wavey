#include "spectrum_window.hpp"

#include <SDL2/SDL.h>

#include <iostream>

#include "imgui.h"
#include "implot.h"
#include "misc/cpp/imgui_stdlib.h"
#include "spectrum_state.hpp"
#include "state.hpp"

namespace {
std::string StrCat(const std::string& a, const std::string& b) {
    return a + b;
}
}  // namespace

SpectrumWindow::SpectrumWindow(SpectrumState* state) : state_(state) {}

void SpectrumWindow::Draw() {
    if (!visible_)
        return;
    ImGui::Begin("Spectrum",
                 &visible_);  // Pass a pointer to our bool variable (the window will have a
                              // closing button that will clear the bool when clicked)
    if (ImPlot::BeginPlot("Spectrum")) {
        float max_xaxis_hz = 0;
        for (const Spectrum& s : state_->spectrums()) {
            max_xaxis_hz = std::max(max_xaxis_hz, *s.frequencies.crbegin());
        }
        ImPlot::SetupAxisScale(ImAxis_X1, log_scale_ ? ImPlotScale_Log10 : ImPlotScale_Linear);
        ImPlot::SetupAxesLimits(log_scale_ ? 10 : 0, max_xaxis_hz, -100, 0);
        ImPlot::SetupAxes("Frequency (Hz)", "Amplitude (dB)");

        for (Spectrum& s : state_->spectrums()) {
            if (s.future_spectrum.valid()) {
                if (s.future_spectrum.wait_for(std::chrono::seconds(0)) ==
                    std::future_status::ready) {
                    s.spectrum = s.future_spectrum.get();
                }
            }
            if (!s.spectrum || !s.visible)
                continue;
            ImPlot::PlotLine(s.name.c_str(), s.frequencies.data(), s.spectrum->data(),
                             s.frequencies.size());
        }
        ImPlot::EndPlot();
    }
    ImGui::Checkbox("Log frequency", &log_scale_);
    ImGui::BeginTable("spectrum_list", 3);
    for (Spectrum& s : state_->spectrums()) {
        ImGui::TableNextColumn();
        ImGui::Checkbox(StrCat("##spectrumVisible", s.name).c_str(), &s.visible);
        ImGui::TableNextColumn();
        ImGui::ColorEdit3(StrCat("##Color:", s.name).c_str(), s.color, ImGuiColorEditFlags_NoInputs);
        ImGui::TableNextColumn();
        ImGui::InputText(StrCat("##spectrumName:", s.name).c_str(), &s.name);
        ImGui::TableNextRow();
    }
    ImGui::EndTable();
    ImGui::End();
}

void SpectrumWindow::AddSpectrumFromTrack(const Track& t, float begin, float end, int channel) {
    std::cerr << "SpectrumWindow: Got Spectrum add channel " << channel << std::endl;
    state_->Add(t, begin, end, channel);
}
