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
    void ZoomInVertical();
    void ZoomOutVertical();
    void ZoomOutFullVertical() { vertical_zoom = 1.f; }
    float VerticalZoom() { return vertical_zoom; }
    void ToggleSingleTrack(std::optional<int> track);
    void ShowAllTracks();
    void ShowSingleTrack(std::optional<int> track);
    void ShowSingleChannel(int track, int channel, int num_channels);
    bool ShowingAllTracks() const;
    void PanTo(float time);
    void PanLeft();
    void PanRight();
    float Left() const { return x_left; }
    float Right() const { return x_right; }
    float Top() const { return y_top; }
    float Bottom() const { return y_bottom; }
    float MaxX() const { return x_max; }
    float GetTime(float x) const;
    int GetTrack(float y) const;
    int GetChannel(float y, int num_channels) const;
    float VerticalZoom() const { return vertical_zoom; }
    void Reset();
    bool DbVerticalScale() const { return db_vertical_scale; };
    void ToggleDbVerticalScale() { db_vertical_scale = !db_vertical_scale; };

   private:
    void Zoom(float x, float factor);
    float x_left = 0.f;
    float x_right = 0.f;
    float x_max = 0.f;
    float y_top = 0.f;
    float y_bottom = 0.f;
    float y_max = 0.f;
    float vertical_zoom = 1.f;
    bool db_vertical_scale = false;
};
#endif
