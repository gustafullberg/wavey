#ifndef SPECTROGRAM_SHADER_HPP
#define SPECTROGRAM_SHADER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>

class SpectrogramShader {
   public:
    void Init();
    void Terminate();
    void Draw(const glm::mat4& mvp);

   private:
    GLuint program = 0;
};

#endif
