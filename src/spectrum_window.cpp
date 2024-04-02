#include "spectrum_window.hpp"

#include <SDL2/SDL.h>

#include <iostream>

#include "imgui.h"
#include "implot.h"
#include "misc/cpp/imgui_stdlib.h"
#include "spectrum_state.hpp"
#include "state.hpp"

namespace {
template <class T>
std::string MakeId(const std::string& prefix, const T& obj) {
    constexpr char kIdFormat[] = "%s##%p";
    int size_s = std::snprintf(nullptr, 0, kIdFormat, prefix.c_str(), &obj);
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, kIdFormat, prefix.c_str(), &obj);
    return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}
}  // namespace

SpectrumWindow::SpectrumWindow(SpectrumState* state) : state_(state) {}

void SpectrumWindow::Draw() {
    if (!visible_)
        return;
    ImGui::SetNextWindowSize(ImVec2(800, 0));
    ImGui::Begin("Spectrum##window", &visible_,
                 ImGuiWindowFlags_NoScrollbar);  // Pass a pointer to our bool variable (the window
                                                 // will have a
    // closing button that will clear the bool when clicked)
    ImGui::BeginGroup();
    if (ImPlot::BeginPlot("Spectrum##graph", ImVec2(-1, 0),
                          ImPlotFlags_NoLegend | ImPlotFlags_Crosshairs | ImPlotFlags_NoChild)) {
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
            // Set the line color and weight for the next item only.
            ImPlot::SetNextLineStyle(ImVec4(s.color[0], s.color[1], s.color[2], 1.0f));
            ImPlot::PlotLine(MakeId(s.name, s).c_str(), s.frequencies.data(), s.spectrum->data(),
                             s.frequencies.size());
        }
        ImPlot::EndPlot();
    }
    ImGui::Checkbox("Log frequency", &log_scale_);
    ImGui::EndGroup();
    ImGui::BeginGroup();
    std::vector<std::list<Spectrum>::iterator> spectrums_to_remove;
    for (auto s_it = state_->spectrums().begin(); s_it != state_->spectrums().end(); ++s_it) {
        Spectrum& s = *s_it;
        if (ImGui::Button(MakeId("D##Delete", s).c_str())) {
            spectrums_to_remove.push_back(s_it);
            continue;
        }
        ImGui::SameLine();
        ImGui::Checkbox(MakeId("##spectrumVisible", s).c_str(), &s.visible);
        ImGui::SameLine();
        ImGui::ColorEdit3(MakeId("##Color", s).c_str(), s.color, ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        ImGui::InputText(MakeId("##spectrumName", s).c_str(), &s.name);
    }
    ImGui::EndGroup();
    ImGui::End();
    for (auto to_be_removed : spectrums_to_remove) {
        state_->Remove(to_be_removed);
    }
}

void SpectrumWindow::AddSpectrumFromTrack(const Track& t, float begin, float end, int channel) {
    std::cerr << "SpectrumWindow: Got Spectrum add channel " << channel << std::endl;
    state_->Add(t, begin, end, channel);
}
