#version 140
#extension GL_ARB_compatibility: enable
in vec3 pos;
in vec3 color;


out vec3 entryPoint;
out vec3 viewPixelPos;

uniform mat4 Mvp;
uniform mat4 Mv;

void main(){
    gl_Position = Mvp * vec4(pos, 1.0);
    entryPoint = color;
    viewPixelPos = vec3(Mv * vec4(pos, 1.0));
}
