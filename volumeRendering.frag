#version 140
#extension GL_ARB_compatibility: enable
#define STEP_COUNT 1600.0f
#define WIDTH 600.0f
#define HEIGHT 600.0f
in vec3 pixelPosition;

//uniform vec3 eyePosition;
//uniform vec3 objectMin;
//uniform vec3 objectMax;
//uniform vec3 up;
uniform sampler3D tex;
uniform sampler2D backFaceTex;
uniform sampler1D transferFuncTex;


void main()
{
    
    // Ray marching set up
    // -------------------
    vec3 entryPoint = pixelPosition;
    vec2 backFaceTexCoord = gl_FragCoord.xy / vec2(WIDTH, HEIGHT);
    vec3 exitPoint = texture(backFaceTex, backFaceTexCoord).xyz;
    if (entryPoint == exitPoint) 
        discard;

    vec4 composedColor = vec4(0,0,0,0);
    vec3 marchDir = normalize(exitPoint - entryPoint);  // normalized marching direction
    float marchLen = length(exitPoint - entryPoint);    // total length of marching
    float dt = marchLen / STEP_COUNT;                   // distance traveled per step i.e. the step size

    vec3 voxelCoord = entryPoint;
    float maxIntensity = texture(tex, voxelCoord).x;    // for MIP (Maximum Intensity Projection)
    
    // Ray marching start
    // ------------------
    for (int i = 0; i < STEP_COUNT; i++) {
        float intensity = texture(tex, voxelCoord).x;   // sampled intensity at voxelCoord
        if (intensity > maxIntensity)
            maxIntensity = intensity;
        vec4 sampledColor = texture(transferFuncTex, intensity);
        voxelCoord = voxelCoord + dt * marchDir;                    // marches forward 
    }
    if (maxIntensity != 0) 
        composedColor = vec4(1, 1, 1, 1/maxIntensity);
   
    // ----------------
    // Ray marching end
    //gl_FragColor = vec4(exitPoint, 0);
    gl_FragColor = vec4(vec3(composedColor) / composedColor.a, 0);
}