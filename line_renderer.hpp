#ifndef LINE_RENDERER_HPP
#define LINE_RENDERER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>

class LineRenderer {
   public:
    void Init();
    void Terminate();
    void Draw(const glm::mat4& mvp,
              const glm::vec2& from,
              const glm::vec2& to,
              const glm::vec4& color);

   private:
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
};

#endif
