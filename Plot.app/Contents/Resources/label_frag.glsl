#version 330 core

in vec2 fTex;

uniform sampler2D tex;

out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, texture(tex, fTex).r);
}
