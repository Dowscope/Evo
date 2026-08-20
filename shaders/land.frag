#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) flat in float emissive;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform DrawState {
    mat4 viewProjection;
    vec4 modelTranslation;
    vec4 sun;
} drawState;

void main() {
    if (emissive > 0.5) {
        outColor = vec4(color, 1.0);
        return;
    }

    vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    vec3 lightDirection = normalize(drawState.sun.xyz - worldPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float daylight = drawState.sun.w;
    float brightness = 0.07 + daylight * (0.18 + diffuse * 0.75);
    outColor = vec4(color * brightness, 1.0);
}
