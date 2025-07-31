#version 330 core

in vec4 vPos;
in vec4 vNorm;

out vec3 fPos, fNorm;
out vec3 fColor;

uniform float normZ;
uniform bool zIsImag;
uniform float axisLength;
uniform mat3 normal;
uniform mat4 model, view, proj;

void main()
{
    vec4 worldPos;

    if (zIsImag) {
        // Imaginary (.w) component is the z-Axis
        float w = (min(1.0, max(-1.0, 2.0 * vPos.z / axisLength)) + 1.0) / 2.0;
        fColor = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), w);
        worldPos = model * vec4(vPos.x, vPos.y, vPos.w, 1.0);
        fNorm = normal * normalize(vec3(vNorm.z, vNorm.w, normZ));
    } else {
        // Real (.z) component is the z-Axis
        float w = (min(1.0, max(-1.0, 2.0 * vPos.w / axisLength)) + 1.0) / 2.0;
        fColor = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), w);
        worldPos = model * vec4(vPos.x, vPos.y, vPos.z, 1.0);
        fNorm = normal * normalize(vec3(vNorm.x, vNorm.y, normZ));
    }

    fPos = vec3(worldPos);

    gl_Position = proj * view * worldPos;
    gl_ClipDistance[0] = min(axisLength - abs(vPos.z), axisLength - abs(vPos.w));
}