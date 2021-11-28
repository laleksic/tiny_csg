A simple shader loading library.

## Usage

To compile a shader, simply call the compile function of a
shader. Errors occurring during compilation will be thrown as
ShaderError.

When loading shaders from a file, use the load_from_file helper
method, which will create a Shader object from a path and its type.

To link the program, create a ShaderProgram from an array of shaders.

## Example

```C++
Shader* vertex;
Shader* fragment;

try {
    vertex = Shader::load_from_file("shaders/vertex_shader.glsl",
                                    GL_VERTEX_SHADER);
    vertex->compile();
}
catch (ShaderError e) {
    char* buffer;
    asprintf(&buffer, "Could not compile vertex shader: %s", e.what());
    string message = buffer;
    delete buffer;

    throw display_error(message.c_str());
}

try {
    fragment = Shader::load_from_file("shaders/fragment_shader.glsl",
                                      GL_FRAGMENT_SHADER);
    fragment->compile();
}
catch (ShaderError e) {
    char* buffer;
    asprintf(&buffer, "Could not compile fragment shader: %s", e.what());
    string message = buffer;
    delete buffer;

    throw display_error(message.c_str());
}

try {
    Shader* shaders[2] = {vertex, fragment};
    program = new ShaderProgram(shaders, 2);
    program->use();
}
catch (ShaderError e) {
    char* buffer;
    asprintf(&buffer, "Could not link program: %s", e.what());
    string message = buffer;
    delete buffer;

    throw display_error(message.c_str());
}
```

## Installation

Run make install in the project root. This will install libshader.a
and libshader.so as well as the header files to your
`$HOME/.local/lib` and `$HOME/.local/include` directories.

Ensure you have set the following environment variables:

```bash
LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"
CPLUS_INCLUDE_PATH="$HOME/.local/include:CPLUS_INCLUDE_PATH"
```

After that, the library may be included using `#include <libshader>`.
