#ifndef ZOOM_WINDOW_HPP
#define ZOOM_WINDOW_HPP

#include <algorithm>
#include <optional>

class ZoomWindow {
   public:
    void LoadFile(float length);
    void ZoomIn(float x) { Zoom(x, 0.75f); }
    void ZoomOut(float x) { Zoom(x, 1.f / 0.75f); }
    void ZoomOutFull() {
        x_left = 0.f;
        x_right = x_max;
    }
    void ZoomRange(float start, float end) {
        if (start > end) {
            std::swap(start, end);
        }
        x_left = std::max(start, 0.f);
        x_right = std::min(end, x_max);
    }
    void ToggleSingleTrack(std::optional<int> track);
    void PanTo(float time);
    float Left() const { return x_left; }
    float Right() const { return x_right; }
    float Top() const { return y_top; }
    float Bottom() const { return y_bottom; }
    float MaxX() const { return x_max; }
    float GetTime(float x) const;
    int GetTrack(float y) const;
    void Reset();

   private:
    void Zoom(float x, float factor);
    float x_left = 0.f;
    float x_right = 0.f;
    float x_max = 0.f;
    float y_top = 0.f;
    float y_bottom = 0.f;
    float y_max = 0.f;
};
#endif
