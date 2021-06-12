#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords0;
layout (location = 3) in vec2 aTexCoords1;

out vec2 TexCoord;
out vec2 TexCoord2;
out vec3 WorldPos;
out vec3 Normal;

uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
};

void main()
{
	TexCoord = aTexCoords0;
	TexCoord2 = aTexCoords1;	

	vec4 pos = model * vec4(aPos, 1.0);
	WorldPos = vec3(pos.xyz) / pos.w;

	Normal = normalize(vec3(model * vec4(aNormal.xyz, 0.0)));

    gl_Position = projection * view * pos;
}