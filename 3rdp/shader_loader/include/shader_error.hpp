#ifndef SHADER_ERROR_HPP
#define SHADER_ERROR_HPP

#include <stdexcept>

class ShaderError: public std::runtime_error
{
public:
    ShaderError(const char* message): std::runtime_error(message) {};
};

#endif
