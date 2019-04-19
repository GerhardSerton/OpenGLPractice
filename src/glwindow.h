#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <GL/glew.h>

#include "geometry.h"

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

class OpenGLWindow
{
public:
    OpenGLWindow();

    void initGL();
    void render();
    bool handleEvent(SDL_Event e);
    void cleanup();


private:
    SDL_Window* sdlWin;

    GLuint vao;
    GLuint vbo;
    GLuint shader;
    GLuint vertexBuffer;
    GLuint colourBuffer;
};

#endif
