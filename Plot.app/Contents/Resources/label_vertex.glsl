#version 330 core

in vec2 vPos;
in vec2 vTex;

out vec2 fTex;

uniform vec2 translate;

void main() {
    fTex = vTex;
    gl_Position = vec4(vPos + translate, 0.0, 1.0);
}