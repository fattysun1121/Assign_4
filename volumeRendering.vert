#version 140
#extension GL_ARB_compatibility: enable
in vec3 pos;
in vec3 color;


out vec3 pixelPosition;
uniform mat4 Mvp;

void main(){
    gl_Position = Mvp * vec4(pos, 1.0);
    pixelPosition = color;
}
