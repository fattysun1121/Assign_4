#version 140
#extension GL_ARB_compatibility: enable
#define STEP_SIZE 0.001f
#define WIN_WIDTH 600.0f
#define WIN_HEIGHT 600.0f
#define MIP 1
#define ISOSURFACE 2
#define ALPHA_COMPO 3

in vec3 pixelPosition;

//uniform vec3 eyePosition;
//uniform vec3 objectMin;
//uniform vec3 objectMax;
uniform vec3 up;
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
    if (entryPoint == exitPoint)                        // no need to process points outside of the bounding box
        discard;
    vec4 composedColor = vec4(0,0,0,0);
    vec3 marchDir = normalize(exitPoint - entryPoint);  // normalized marching direction
    float marchLen = length(exitPoint - entryPoint);    // total length of marching
    float dt = STEP_SIZE;  // distance traveled per step
    float distanceTraveled = 0;  // total distance traveled so far
    vec3 voxelCoord = entryPoint;

    // Different ray marching algorithms based on selected rendering mode (MIP, isosurface processing, alpha compositioning)
    // ---------------------------------------------------------------------------------------------------------------------
    if (mode == MIP) {
        float maxIntensity = texture(tex, voxelCoord).x;    
       
        while (distanceTraveled < marchLen) {
            float intensity = texture(tex, voxelCoord).x;   // sampled intensity at voxelCoord
            if (intensity > maxIntensity)
                maxIntensity = intensity;
      
            voxelCoord = voxelCoord + dt * marchDir;        // marches forward 
            distanceTraveled += dt;
        }
     
        composedColor = vec4(maxIntensity, maxIntensity, maxIntensity, 0);
    }
    else if (mode == ALPHA_COMPO) {
        vec3 bgColor = vec3(0.0, 0.0, 0.0);
        while (true) {             
            float intensity = texture(tex, voxelCoord).x;  // sampled intensity at voxelCoord
            vec4 transFuncOutput = texture(transferFuncTex, intensity);  // Transfer function sampled RGBA
            
            
            transFuncOutput.a = pow(transFuncOutput.a, 10);
            composedColor.rgb += (1 - composedColor.a) * transFuncOutput.rgb;
            composedColor.a += (1 - composedColor.a) * transFuncOutput.a;
            
            
            voxelCoord += dt * marchDir;  // marches forward 
            distanceTraveled += dt;
            if (composedColor.a >= 0.95) {
                composedColor.a = 1.0;
                break;
            }
            if (distanceTraveled > marchLen) {
                composedColor.rgb = composedColor.rgb * composedColor.a + (1 - composedColor.a) * bgColor;
                break;
            }
           
            
        }

    } 
    else if (mode == ISOSURFACE) {
        while (distanceTraveled < marchLen) {
            float intensity = texture(tex, voxelCoord).x;   // sampled intensity at voxelCoord
            
      
            voxelCoord = voxelCoord + dt * marchDir;        // marches forward 
            distanceTraveled += dt;
        }
     
    }
   

    gl_FragColor = composedColor;
  

     
}