#ifndef LABEL_RENDERER_HPP
#define LABEL_RENDERER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include "gpu_label.hpp"
#include "shader.hpp"

class LabelRenderer {
   public:
    void Init();
    void Terminate();
    void Draw(const GpuLabel& label,
              float x,
              float y,
              float win_width,
              float win_height,
              float scale_factor,
              const glm::vec4& color_text,
              const glm::vec4& color_shadow,
              bool centered_h);

   private:
    GLuint vao = 0;
    GLuint vbo = 0;

    class LabelShader : public Shader {
       public:
        void Init();
        void Draw(const glm::mat4& mvp, const glm::vec4& color);
    } shader;
};

#endif
