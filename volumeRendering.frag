#version 140
#extension GL_ARB_compatibility: enable
#define STEP_SIZE 0.001f
#define WIN_WIDTH 600.0f
#define WIN_HEIGHT 600.0f
#define MIP 1
#define ISOSURFACE 2
#define ALPHA_COMPO 3

in vec3 entryPoint;
in vec3 viewPixelPos;
in vec3 viewLightPos;
uniform mat4 Mv;
uniform sampler3D tex;
uniform sampler2D backFaceTex;
uniform sampler1D transferFuncTex;
uniform int mode;
uniform int transparency;

vec3 applyPhong(vec3 position, vec3 normal, vec3 objColor) {
    vec3 Ld = vec3(1.0, 0.0, 1.0);
    vec3 Ls = vec3(0.0, 1.0, 1.0);
    vec3 La = vec3(1.0, 1.0, 0.0);
    float Kd = 1.0, Ks = 0.5, Ka = 0.2;

    float alpha = 32;
    vec3 lightSource = viewLightPos;

    vec3 n = normalize(normal);
	vec3 l = normalize(lightSource - position);

	// diffuse
	float diff = max(0.0, dot(l, n));
	vec3 diffuse = Kd * diff * Ld;
	
	// specular
	vec3 viewSource = normalize(-position);
	vec3 reflectSource = normalize(reflect(-l, n));
	float spec = pow(max(0.0, dot(viewSource, reflectSource)), alpha);
	vec3 specular = Ks * spec * Ls;

	// ambient
	vec3 ambient = Ka * La;
	vec3 color = diffuse * objColor;
    
	return color;
}

float getIntensity(vec3 coord) {
    return texture(tex, coord).x;
}

void main()
{
    
    // Ray marching set up
    // -------------------
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
        float maxIntensity = getIntensity(voxelCoord);    
       
        while (distanceTraveled < marchLen) {
            float intensity = getIntensity(voxelCoord);   // sampled intensity at voxelCoord
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
            float intensity = getIntensity(voxelCoord);  // sampled intensity at voxelCoord
            vec4 transFuncOutput = texture(transferFuncTex, intensity);  // Transfer function sampled RGBA
            
            
            transFuncOutput.a = pow(transFuncOutput.a, transparency);
            composedColor.rgb += (1 - composedColor.a) * transFuncOutput.rgb;
            composedColor.a += (1 - composedColor.a) * transFuncOutput.a;
            
            
            voxelCoord += dt * marchDir;  // marches forward 
            distanceTraveled += dt;

            if (composedColor.a >= 0.95) 
                break;
            
            if (distanceTraveled > marchLen) {
                composedColor.rgb = composedColor.rgb * composedColor.a + (1 - composedColor.a) * bgColor;
                break;
            }   
        }

    } 
    else if (mode == ISOSURFACE) {
        float isoThreshold = 120.0/255.0;
        while (distanceTraveled < marchLen) {            
            // Found the zero-crossing location
            float intensity = getIntensity(voxelCoord);
            if (getIntensity(voxelCoord) > isoThreshold) {
                // compute gradient 
                vec4 gradient = vec4(0.0);
                gradient.x = (getIntensity(voxelCoord - vec3(1, 0, 0)) - getIntensity(voxelCoord + vec3(1, 0, 0))) / 2.0;
                gradient.y = (getIntensity(voxelCoord - vec3(0, 1, 0)) - getIntensity(voxelCoord + vec3(0, 1, 0))) / 2.0;
                gradient.z = (getIntensity(voxelCoord - vec3(0, 0, 1)) - getIntensity(voxelCoord + vec3(0, 0, 1))) / 2.0;
                // phong shading
                vec3 normal = vec3(Mv * gradient);  // converts normal to from world to view space
                composedColor.rgb = applyPhong(viewPixelPos, normal, vec3(texture(transferFuncTex, intensity)));
                break;
            }
            voxelCoord = voxelCoord + dt * marchDir;        // marches forward 
            distanceTraveled += dt;
        }
     
    }
   
    gl_FragColor = composedColor;  
}