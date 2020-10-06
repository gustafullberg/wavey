#ifndef ZOOM_WINDOW_HPP
#define ZOOM_WINDOW_HPP

class ZoomWindow {
   public:
    void LoadFile(float length);
    void ZoomIn(float x) { Zoom(x, 0.75f); }
    void ZoomOut(float x) { Zoom(x, 1.f / 0.75f); }
    void PanLeft();
    void PanRight();
    float Left() const { return x_left; }
    float Right() const { return x_right; }
    float Top() const { return y_top; }
    float Bottom() const { return y_bottom; }
    float GetTime(float x) const;
    float GetX(float time) const;
    float GetY(float logic_y) const;
    int GetTrack(float y) const;

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
