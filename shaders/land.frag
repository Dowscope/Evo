#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) flat in float emissive;
layout(location = 3) in float surfaceTemperature;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform DrawState {
    mat4 viewProjection;
    vec4 modelTranslation;
    vec4 sun;
    vec4 display;
} drawState;

vec3 temperatureColor(float celsius) {
    float normalized = clamp((celsius + 10.0) / 60.0, 0.0, 1.0);
    if (normalized < 0.333333) {
        return mix(vec3(0.05, 0.15, 0.85), vec3(0.0, 0.85, 0.9), normalized * 3.0);
    }
    if (normalized < 0.666667) {
        return mix(vec3(0.0, 0.85, 0.9), vec3(1.0, 0.9, 0.05),
                   (normalized - 0.333333) * 3.0);
    }
    return mix(vec3(1.0, 0.9, 0.05), vec3(0.85, 0.05, 0.02),
               (normalized - 0.666667) * 3.0);
}

void main() {
    if (emissive > 0.5) {
        outColor = vec4(color, 1.0);
        return;
    }

    if (drawState.display.x > 0.5) {
        outColor = vec4(temperatureColor(surfaceTemperature), 1.0);
        return;
    }

    vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
    if (!gl_FrontFacing) {
        normal = -normal;
    }
    vec3 lightDirection = normalize(drawState.sun.xyz);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float daylight = drawState.sun.w;
    float brightness = 0.07 + daylight * (0.18 + diffuse * 0.75);
    outColor = vec4(color * brightness, 1.0);
}
