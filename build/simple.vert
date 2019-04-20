#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 Projection;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Scale;
uniform mat4 Rotation;
uniform mat4 Translation;

void main()
{
    mat4 newMVP = Projection * View * Model * Scale * Rotation * Translation;
    gl_Position = newMVP *  vec4(position,1.0f);
}
