#ifndef QUAD_RENDERER_HPP
#define QUAD_RENDERER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>

class QuadRenderer {
   public:
    void Init();
    void Terminate();
    void Draw(float x, float y, float size_x, float size_y);

   private:
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
};

#endif
