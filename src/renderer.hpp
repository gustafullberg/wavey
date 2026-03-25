#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <functional>
#include <glm/glm.hpp>
#include <memory>

class State;

class Renderer {
   public:
    virtual ~Renderer();
    virtual void Draw(
        State* state,
        int win_width,
        int win_height,
        int ui_bottom,
        float timeline_height,
        float scale_factor,
        bool view_spectrogram,
        bool view_bark_scale,
        bool playing,
        float play_time,
        const std::function<void(float, bool selected, const char*)>& label_print_func,
        const std::function<void(float, const char*)>& time_print_func) = 0;
  static std::unique_ptr<Renderer> Create();
};

#endif
