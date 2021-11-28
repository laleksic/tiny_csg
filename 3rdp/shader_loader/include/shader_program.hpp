#ifndef SHADER_PROGRAM_HPP
#define SHADER_PROGRAM_HPP

#include <GL/glu.h>
#ifndef SHADER_HPP
#include "shader.hpp"
#endif

class ShaderProgram
{
    GLuint id;

public:
    ShaderProgram (Shader** shaders, GLuint num);
    ~ShaderProgram ();
    void use ();
    GLuint getId ();
};

#endif
