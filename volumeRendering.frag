#version 140
#extension GL_ARB_compatibility: enable

in vec3 pixelPosition;
in vec3 Color;

uniform vec3 eyePosition;
uniform vec3 objectMin;
uniform vec3 objectMax;
uniform vec3 up;
uniform sampler3D tex;
uniform sampler2D backFaceTex;
uniform sampler1D transferFunction;


void main()
{
    // find entry and exit points
    vec3 entryPoint = pixelPosition;
    vec3 exitPoint = texture(backFaceTex, vec2(pixelPosition[0], pixelPosition[1])).xyz;
    vec4 composedColor=vec4(0,0,0,0);

    if (entryPoint == exitPoint) 
        discard;
 
    // Ray marching
    vec3 marchDir = exitPoint - entryPoint;
    int numOfSteps = 1000;


    
    gl_FragColor = vec4(Color, 1.0);
}