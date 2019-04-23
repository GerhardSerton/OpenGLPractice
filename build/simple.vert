#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 Projection;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Scale;
uniform mat4 Rotation;
uniform mat4 Translation;
uniform mat4 NegaTranslate;

void main()
{
    mat4 newRotation = Translation * Rotation * NegaTranslate;
    mat4 newMVP = Projection * View * Model * newRotation * Translation * Scale;
    gl_Position = newMVP *  vec4(position,1.0f);
}
