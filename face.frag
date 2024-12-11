#version 140
#extension GL_ARB_compatibility: enable

in vec3 Color;
void main()
{
    
    gl_FragColor = vec4(Color, 1.0);
}
