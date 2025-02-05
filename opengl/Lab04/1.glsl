#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 proj;

out vec4 particleColor;

void main() {
    gl_Position = proj * view * vec4(position, 1.0);
    particleColor = vec4(0.0, 0.5, 1.0, 1.0); // Ë®À¶É«
}
