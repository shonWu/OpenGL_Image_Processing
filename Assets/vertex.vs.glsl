#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);
uniform int mode = 0;

out VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space view vector
    vec2 texcoord;
	vec3 normal;
} vertexData;

void main()
{	

	// ppt Shading :Code: Vertex Shader (Cont¡¦d)ªº¤º®e
	// Calculate view-space coordinate
	vec4 P = um4mv * vec4(iv3vertex, 1.0);

	// Calculate normal in view-space
	// Using only the rotational components of mv_matrix
	vertexData.N = mat3(um4mv) * iv3normal;

	// Calculate light vector
	vertexData.L = light_pos - P.xyz;

	// Calculate view vector
	vertexData.V = -P.xyz;
    
	// Calculate the clip-space position of each vertex
	gl_Position = um4p * P;


	// scene input data
	vertexData.texcoord = iv2tex_coord;
	vertexData.normal = iv3normal;
}

