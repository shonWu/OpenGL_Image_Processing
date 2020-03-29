#version 410

// output
layout(location = 0) out vec4 fragColor;


uniform mat4 um4mv;
uniform mat4 um4p;
uniform int mode = 0;
uniform int bar_x=300;

// input from vertex shader
in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
	vec3 normal;
} fs_in;

uniform sampler2D us2dtex;
uniform vec3 specular_albedo = vec3(1.0);
uniform float specular_power = 3000.0;

float sigma_e = 2.0f; 
float sigma_r = 2.8f; 
float phi = 3.4f; 
float tau = 0.99f; 
float twoSigmaESquared = 2.0 * sigma_e * sigma_e;		
float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;		
int halfWidth = int(ceil(2.0 * sigma_r)); 
vec2 img_size = vec2(1024, 611); 
int nbins = 8; 

#define SORT_SIZE  49  
#define MASK_SIZE  7


vec4  Abstraction()
{
	vec4 color = vec4(0);
	int n = 0;
	int half_size = 3;
	for (int i = -half_size; i <= half_size; ++i) {

		for (int j = -half_size; j <= half_size; ++j) {

			vec4 c = texture(us2dtex, fs_in.texcoord + vec2(i, j) / img_size);
			color += c;
			n++;
		}
	}
	color /= n;

	float r = floor(color.r * float(nbins)) / float(nbins);
	float g = floor(color.g * float(nbins)) / float(nbins);
	float b = floor(color.b * float(nbins)) / float(nbins);
	color = vec4(r, g, b, color.a);

	vec2 sum = vec2(0.0);
	vec2 norm = vec2(0.0);
	int kernel_count = 0;
	for (int i = -halfWidth; i <= halfWidth; ++i) {

		for (int j = -halfWidth; j <= halfWidth; ++j) {

			float d = length(vec2(i, j));
			vec2 kernel = vec2(exp(-d * d / twoSigmaESquared),
				exp(-d * d / twoSigmaRSquared));
			vec4 c = texture(us2dtex, fs_in.texcoord + vec2(i, j) / img_size);
			vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);

			norm += 2.0 * kernel;
			sum += kernel * L;
		}
	}
	sum /= norm;

	float H = 100.0 * (sum.x - tau * sum.y);
	float edge = (H > 0.0) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H);

	color *= vec4(edge, edge, edge, 1.0);
	return color;
}



void main()
{
/*
	// ppt shadingªº¤º®e: Code: Fragment Shader (Cont¡¦d)
	// Normalize the incoming N¡BL and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);
    
    vec3 texColor = texture(us2dtex,fs_in.texcoord).rgb;

	// Compute the diffuse and specular components sfor each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * texColor;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;
*/
	vec4 color = texture(us2dtex, fs_in.texcoord);
	fragColor = color;

	
	if (gl_FragCoord.x < bar_x){
		vec4 tex_color = texture(us2dtex ,fs_in.texcoord);
		fragColor = tex_color;
	}
	else if(gl_FragCoord.x >= bar_x && gl_FragCoord.x <= bar_x+10){
		fragColor = vec4(1.0 ,0.0 ,0.0 ,1.0);
	}
	else{
	// write final color to the framebuffer
		if(mode == 0){
			vec4 color = texture(us2dtex, fs_in.texcoord);
			fragColor = color;
		}
		if(mode == 1){
			fragColor = vec4(fs_in.normal ,1.0);
		}
	
		if(mode == 2){
			fragColor = Abstraction();
		}
	}
}

