#version 330 core

in vec2 fTex;

out vec4 outColor;

uniform sampler2D tex;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, texture(tex, fTex).r);
}
