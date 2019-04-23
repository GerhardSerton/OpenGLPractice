#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>

#include "glwindow.h"
#include "geometry.h"

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

using namespace std;

const char* glGetErrorString(GLenum error)
{
    switch(error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

void glPrintError(const char* label="Unlabelled Error Checkpoint", bool alwaysPrint=false)
{
    GLenum error = glGetError();
    if(alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
    FILE* shaderFile = fopen(shaderFilename, "r");
    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderText = new char[shaderSize+1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
                       const char* fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
}


void OpenGLWindow::initGL()
{
    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL);
    if(!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if(glewInitResult != GLEW_OK)
    {
        const GLubyte* errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0,0,0,1);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Note that this path is relative to your working directory
    // when running the program (IE if you run from within build
    // then you need to place these files in build as well)
    shader = loadShaderProgram("simple.vert", "simple.frag");
    glUseProgram(shader);

    int colorLoc = glGetUniformLocation(shader, "objectColour");
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);

    // Load the model that we want to use and buffer the vertex attributes

    GeometryData shape;
    shape.loadFromOBJFile(currentObjectFile);
    vertexCount = shape.vertexCount();
    cout << "Vertex count: " << shape.vertices.size() << "\n";
    cout << "Texture coords count: " << shape.textureCoords.size() << "\n";
    cout << "Normals count: " << shape.normals.size() << "\n";
    cout << "Tangents count: " << shape.tangents.size() << "\n";
    cout << "BiTangents count: " << shape.bitangents.size() << "\n";
    cout << "Faces count: " << shape.faces.size() << "\n";

    GLfloat vertexBufferData [shape.vertices.size()];
    copy(shape.vertices.begin(), shape.vertices.end(), vertexBufferData);


    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);

    glm::mat4 Projection = glm::perspective(glm::radians(35.0f), (float) 640 / (float) 480, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(glm::vec3(4,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 Scale = glm::mat4(1.0f);
    glm::mat4 Rotation = glm::mat4(1.0f);
    glm::mat4 Translation = glm::mat4(1.0f);
    glm::mat4 NegaTranslate = glm::mat4(1.0f);

    GLuint ProjID = glGetUniformLocation(shader, "Projection");
    GLuint ViewID = glGetUniformLocation(shader, "View");
    GLuint ModelID = glGetUniformLocation(shader, "Model");
    GLuint ScaleID = glGetUniformLocation(shader, "Scale");
    GLuint RotateID = glGetUniformLocation(shader, "Rotation");
    GLuint TranslateID = glGetUniformLocation(shader, "Translation");
    GLuint NegaTranslateID = glGetUniformLocation(shader, "NegaTranslate");

    glUniformMatrix4fv(ProjID, 1, GL_FALSE, &Projection[0][0]);
    glUniformMatrix4fv(ViewID, 1, GL_FALSE, &View[0][0]);
    glUniformMatrix4fv(ModelID, 1, GL_FALSE, &Model[0][0]);
    glUniformMatrix4fv(ScaleID, 1, GL_FALSE, &Scale[0][0]);
    glUniformMatrix4fv(RotateID, 1, GL_FALSE, &Rotation[0][0]);
    glUniformMatrix4fv(TranslateID, 1, GL_FALSE, &Translation[0][0]);
    glUniformMatrix4fv(NegaTranslateID, 1, GL_FALSE, &NegaTranslate[0][0]);
    //3glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    glPrintError("Setup complete", true);
}

void OpenGLWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Rotate
    if (currentFuction == 0)
    {
      int x, y;
      SDL_GetMouseState(&x, &y);

      glm::mat4 Model = glm::mat4(1.0f);
      Model = glm::rotate(Model, glm::radians((float)x), glm::vec3(0.0f, 0.0f, 1.0f));
      glm::mat4 Model2 = glm::mat4(1.0f);
      Model2 = glm::rotate(Model2, glm::radians((float)y), glm::vec3(0.0f, 1.0f, 0.0f));

      glm::mat4 finalModel = Model * Model2;

      GLuint ModelID = glGetUniformLocation(shader, "Rotation");
      glUniformMatrix4fv (ModelID, 1, GL_FALSE, &finalModel[0][0]);

    }
    //Scale the model
    else if (currentFuction == 1)
    {
      int x, y;
      SDL_GetMouseState(&x, &y);

      glm::mat4 Model = glm::mat4(1.0f);
      Model = glm::scale(Model, glm::vec3((float)x/100, (float)x/100, (float)x/100));

      GLuint ModelID = glGetUniformLocation(shader, "Scale");
      glUniformMatrix4fv (ModelID, 1, GL_FALSE, &Model[0][0]);
    }
    //Translate the model
    else if (currentFuction == 2)
    {
      int x, y;
      SDL_GetMouseState(&x, &y);

      glm::mat4 Model = glm::mat4(1.0f);
      Model = glm::translate(Model, glm::vec3((float)x/100, (float)y/100, 0.0));


      glm::mat4 Inverse = glm::mat4(1.0f);
      Inverse = glm::translate(Inverse, glm::vec3((float)x/-100, (float)y/-100, 0.0));

      GLuint ModelID = glGetUniformLocation(shader, "Translation");
      GLuint InverseID = glGetUniformLocation(shader, "NegaTranslate");

      glUniformMatrix4fv (ModelID, 1, GL_FALSE, &Model[0][0]);
      glUniformMatrix4fv (InverseID, 1, GL_FALSE, &Inverse[0][0]);
    }
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glDisableVertexAttribArray(0);

    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if(e.type == SDL_KEYDOWN)
    {
        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }
        if (e.key.keysym.sym == SDLK_DOWN)
        {
          int colorLoc = glGetUniformLocation(shader, "objectColour");
          glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);
        }
        if (e.key.keysym.sym == SDLK_UP)
        {
          int colorLoc = glGetUniformLocation(shader, "objectColour");
          glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
        }
        //Rotate the model
        if (e.key.keysym.sym == SDLK_p)
        {
          currentFuction = 0;

        }
        //Scale the model
        if (e.key.keysym.sym == SDLK_s)
        {
          currentFuction = 1;
        }
        //Translate the model
        if (e.key.keysym.sym == SDLK_t)
        {
          currentFuction = 2;
        }
        //Reset the model
        if (e.key.keysym.sym == SDLK_r)
        {
          glm::mat4 Projection = glm::perspective(glm::radians(35.0f), (float) 640 / (float) 480, 0.1f, 100.0f);
          glm::mat4 View = glm::lookAt(glm::vec3(4,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
          glm::mat4 Model = glm::mat4(1.0f);
          glm::mat4 Scale = glm::mat4(1.0f);
          glm::mat4 Rotation = glm::mat4(1.0f);
          glm::mat4 Translation = glm::mat4(1.0f);
          glm::mat4 NegaTranslate = glm::mat4(1.0f);

          GLuint ProjID = glGetUniformLocation(shader, "Projection");
          GLuint ViewID = glGetUniformLocation(shader, "View");
          GLuint ModelID = glGetUniformLocation(shader, "Model");
          GLuint ScaleID = glGetUniformLocation(shader, "Scale");
          GLuint RotateID = glGetUniformLocation(shader, "Rotation");
          GLuint TranslateID = glGetUniformLocation(shader, "Translation");
          GLuint NegaTranslateID = glGetUniformLocation(shader, "NegaTranslate");

          glUniformMatrix4fv(ProjID, 1, GL_FALSE, &Projection[0][0]);
          glUniformMatrix4fv(ViewID, 1, GL_FALSE, &View[0][0]);
          glUniformMatrix4fv(ModelID, 1, GL_FALSE, &Model[0][0]);
          glUniformMatrix4fv(ScaleID, 1, GL_FALSE, &Scale[0][0]);
          glUniformMatrix4fv(RotateID, 1, GL_FALSE, &Rotation[0][0]);
          glUniformMatrix4fv(TranslateID, 1, GL_FALSE, &Translation[0][0]);
          glUniformMatrix4fv(NegaTranslateID, 1, GL_FALSE, &NegaTranslate[0][0]);
        }

        if (e.key.keysym.sym == SDLK_z)
        {
          GeometryData shape;
          shape.loadFromOBJFile(currentObjectFile);
          GLfloat vertexBufferData [shape.vertices.size()*2];
          for (int i = 0; i < shape.vertices.size(); ++i)
          {
            vertexBufferData[i] = shape.vertices[i];
          }
          for (int j = 0; j < shape.vertices.size(); ++j)
          {
            vertexBufferData[shape.vertices.size() + j] = shape.vertices[j] + 1;
          }

          copy(shape.vertices.begin(), shape.vertices.end(), vertexBufferData);

          glGenBuffers(1, &vbo);
          glBindBuffer(GL_ARRAY_BUFFER, vbo);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);
        }
    }
    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    SDL_DestroyWindow(sdlWin);
}
