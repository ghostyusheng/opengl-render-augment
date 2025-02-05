#version 330 core

in vec4 particleColor;
out vec4 fragColor;

void main() {
    fragColor = particleColor; // 使用传入的颜色
}
