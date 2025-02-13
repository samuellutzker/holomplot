#version 330 core

in vec3 fNorm, fPos;
in vec3 fColor;

out vec4 outColor;

uniform float fLinear, fQuadratic;
uniform vec3 camPos;
uniform vec3 staticColor;
uniform float staticColorMix;

void main() {

    float dist = length(camPos - fPos);
    vec3 camDir = (camPos - fPos) / dist;

    float intensity = abs(dot(camDir, fNorm)) / (1.0 + 0.25 * fLinear * dist + 0.5 * fQuadratic * dist * dist);
    
    outColor = vec4(mix(fColor * intensity, staticColor, staticColorMix), 1.0);
}
