#version 140
#extension GL_ARB_compatibility: enable
#define STEP_COUNT 1600.0f
#define WIN_WIDTH 600.0f
#define WIN_HEIGHT 600.0f

#define MIP 1
#define ISOSURFACE 2
#define ALPHA_COMPO 3

in vec3 pixelPosition;

//uniform vec3 eyePosition;
//uniform vec3 objectMin;
//uniform vec3 objectMax;
//uniform vec3 up;
uniform sampler3D tex;
uniform sampler2D backFaceTex;
uniform sampler1D transferFuncTex;
uniform int mode;
uniform int transparency;

void main()
{
    
    // Ray marching set up
    // -------------------
    vec3 entryPoint = pixelPosition;
    vec2 backFaceTexCoord = gl_FragCoord.xy / vec2(WIN_WIDTH, WIN_HEIGHT);
    vec3 exitPoint = texture(backFaceTex, backFaceTexCoord).xyz;
    if (entryPoint == exitPoint) 
        discard;
    vec4 composedColor = vec4(0,0,0,0);
    vec3 marchDir = normalize(exitPoint - entryPoint);  // normalized marching direction
    float marchLen = length(exitPoint - entryPoint);    // total length of marching
    float dt = marchLen / STEP_COUNT;                   // distance traveled per step i.e. the step size
    vec3 voxelCoord = entryPoint;

    // Different ray marching algorithms based on selected rendering mode
    // ------------------------------------------------------------------
    if (mode == MIP) {
        float maxIntensity = texture(tex, voxelCoord).x;    

        for (int i = 0; i < STEP_COUNT; i++) {
            float intensity = texture(tex, voxelCoord).x;   // sampled intensity at voxelCoord
            if (intensity > maxIntensity)
                maxIntensity = intensity;
      
            voxelCoord = voxelCoord + dt * marchDir;        // marches forward 
        }

        composedColor = vec4(maxIntensity, maxIntensity, maxIntensity, 0);
    }
    else if (mode == ALPHA_COMPO) {
        

        for (int i = 0; i < STEP_COUNT; i++) {                          // Note that this algo starts from i = 1
            float intensity = texture(tex, voxelCoord).x;               // sampled intensity at voxelCoord
            vec4 transFuncOutput = texture(transferFuncTex, intensity); // Transfer function sampled RGBA
            vec3 currColor = transFuncOutput.rgb;
            float currAlpha = transFuncOutput.a;
            if (currAlpha > 0) {
                composedColor = vec4(
                    composedColor.rgb + (1 - composedColor.a) * currColor,  // accumulated color
                    composedColor.a + (1 - composedColor.a) * currAlpha);   // accumulated alpha
            }
            
            if (composedColor.a >= 1.0)
                break;
            voxelCoord = voxelCoord + dt * marchDir;                    // marches forward 
        }
    }

    gl_FragColor = composedColor;
  

     
}