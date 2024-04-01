#ifndef SPECTRUM_WINDOW_HPP_
#define SPECTRUM_WINDOW_HPP_

#include <SDL2/SDL.h>

#include "state.hpp"

class SpectrumState;

class SpectrumWindow {
   public:
    explicit SpectrumWindow(SpectrumState* state);
  ~SpectrumWindow() = default;

    void SetVisible(bool visible) { visible_ = true; }
    void Draw();

    void AddSpectrumFromTrack(const Track& t, float begin, float end, int channel);

   private:
    SpectrumState* state_ = nullptr;

    bool visible_ = false;
    bool log_scale_ = true;

    SDL_Window* window_;
    SDL_GLContext gl_context_;
};

#endif
