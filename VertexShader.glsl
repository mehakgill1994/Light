#version 410

layout(location = 0) in vec2 position;

out vec2 uv;

void main()
{
	gl_Position = vec4(position, 0.5, 1.0);
	uv = position;
}