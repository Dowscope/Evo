#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in float inSurfaceTemperature;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 worldPosition;
layout(location = 2) flat out float emissive;
layout(location = 3) out float surfaceTemperature;

layout(push_constant) uniform DrawState {
    mat4 viewProjection;
    vec4 modelTranslation;
    vec4 sun;
    vec4 display;
} drawState;

void main() {
    worldPosition = inPosition + drawState.modelTranslation.xyz;
    gl_Position = drawState.viewProjection * vec4(worldPosition, 1.0);
    color = inColor;
    emissive = drawState.modelTranslation.w;
    surfaceTemperature = inSurfaceTemperature;
}
