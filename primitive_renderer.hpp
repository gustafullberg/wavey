#ifndef PRIMITIVE_RENDERER_HPP
#define PRIMITIVE_RENDERER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include "shader.hpp"

class PrimitiveRenderer {
   public:
    void Init();
    void Terminate();
    void DrawLine(const glm::mat4& mvp,
                  const glm::vec2& from,
                  const glm::vec2& to,
                  const glm::vec4& color);
    void DrawQuad(const glm::mat4& mvp,
                  const glm::vec2& from,
                  const glm::vec2& to,
                  const glm::vec4& color);

   private:
    void Draw(const glm::mat4& mvp,
              const glm::vec2& from,
              const glm::vec2& to,
              const glm::vec4& color,
              GLenum mode,
              int count);
    GLuint vao = 0;
    GLuint vbo = 0;

    class PrimitiveShader : public Shader {
       public:
        void Init();
        void Draw(const glm::mat4& mvp, const glm::vec4& color);
    } shader;
};

#endif
