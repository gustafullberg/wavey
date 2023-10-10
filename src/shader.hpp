#ifndef SHADER_HPP
#define SHADER_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <string>

class Shader {
   public:
    virtual ~Shader() = default;
    void Init(const std::string& vertex_shader_source,
                      const std::string& fragment_shader_source);
    void Init(const std::string& vertex_shader_source,
                      const std::string& geometry_shader_source,
                      const std::string& fragment_shader_source);
    virtual void Terminate();

   protected:
    GLuint program = 0;
};

#endif
