#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform DrawState {
    mat4 viewProjection;
    vec4 modelTranslation;
    vec4 sun;
    vec4 display;
} drawState;

void main() {
    vec3 worldPosition = inPosition + drawState.modelTranslation.xyz;
    gl_Position = drawState.viewProjection * vec4(worldPosition, 1.0);
}
