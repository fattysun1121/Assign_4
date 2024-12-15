#version 140
#extension GL_ARB_compatibility: enable
in vec3 pos;
in vec3 color;


out vec3 entryPoint;
out vec3 viewPixelPos;
out vec3 viewLightPos;

uniform mat4 Mvp;
uniform mat4 Mv;
uniform mat4 V;
uniform vec3 lightPos;

void main(){
    gl_Position = Mvp * vec4(pos, 1.0);
    entryPoint = color;
    viewPixelPos = vec3(Mv * vec4(pos, 1.0));
    viewLightPos = vec3(V * vec4(lightPos, 1.0));
}
