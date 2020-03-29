#version 410

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

uniform int mode = 0;

out VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

void main()
{
	gl_Position = vec4(position ,1.0 ,1.0);
    vertexData.texcoord = texcoord;
}