#ifndef RENDERER_HPP
#define RENDERER_HPP

class State;

class Renderer {
   public:
    virtual ~Renderer();
    virtual void Draw(State* state,
                      int win_width,
                      int win_height,
                      float scale_factor,
                      bool view_spectrogram,
                      bool view_bark_scale,
                      bool playing,
                      float play_time) = 0;
    virtual float TimelineHeight() = 0;
    static Renderer* Create();
};

#endif
