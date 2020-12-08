#include "zoom_window.hpp"
#include <algorithm>
#include <cmath>

void ZoomWindow::LoadFile(float length) {
    x_max = std::max(x_max, length);
    y_max += 1.f;
    x_left = 0.f;
    x_right = x_max;
    y_top = 0.f;
    y_bottom = y_max;
}

void ZoomWindow::Zoom(float x, float factor) {
    float zoom_level = x_right - x_left;
    if (zoom_level <= 0.f)
        return;
    factor = std::max(factor, 0.001f / zoom_level);
    float focus = x_left + zoom_level * x;
    x_left = focus - zoom_level * factor * x;
    x_right = focus + zoom_level * factor * (1.f - x);

    x_left = std::max(x_left, 0.f);
    x_right = std::min(x_right, x_max);
}

void ZoomWindow::ZoomInVertical() {
    vertical_zoom = std::min(vertical_zoom * 1.2f, 100.f);
}

void ZoomWindow::ZoomOutVertical() {
    vertical_zoom = std::max(vertical_zoom / 1.2f, 1.f);
}

void ZoomWindow::ToggleSingleTrack(std::optional<int> track) {
    if (!ShowingAllTracks()) {
        y_top = 0.f;
        y_bottom = y_max;
    } else if (track) {
        y_top = *track;
        y_bottom = *track + 1;
    }
}

void ZoomWindow::ShowSingleTrack(std::optional<int> track) {
    if (track) {
        y_top = *track;
        y_bottom = *track + 1;
    }
}

bool ZoomWindow::ShowingAllTracks() const {
    return y_top == 0.f && y_bottom == y_max;
}

void ZoomWindow::PanTo(float time) {
    const float zoom_level = x_right - x_left;
    x_left = time;
    x_right = time + zoom_level;
}

float ZoomWindow::GetTime(float x) const {
    return x_left + x * (x_right - x_left);
}

int ZoomWindow::GetTrack(float y) const {
    if (y_top == y_bottom)
        return 0;
    return std::floor(y_top + y * (y_bottom - y_top));
}

void ZoomWindow::Reset() {
    x_left = 0.f;
    x_right = 0.f;
    x_max = 0.f;
    y_top = 0.f;
    y_bottom = 0.f;
    y_max = 0.f;
}
