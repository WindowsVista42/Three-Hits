#version 450

layout(set = 0, binding = 0) readonly uniform ViewProjection {
    mat4 view_proj;
};

struct Light {
    vec4 position_falloff;
    vec4 color_alpha;
};

layout(set = 0, binding = 2, std140) uniform Lights {
    Light lights[6];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_model_position_rotation;
layout(location = 4) in vec4 in_model_color;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec4 out_model_color;
layout(location = 4) out vec4 out_lighting_color_alpha;

mat4 rotation_z(float angle) {
    return mat4(
        cos(angle), -sin(angle), 0, 0,
        sin(angle),  cos(angle), 0, 0,
        0,           0,          1, 0,
        0,           0,          0, 1
    );
}

void main() {
    vec3 model_position = in_model_position_rotation.xyz;
    float model_rotation = in_model_position_rotation.w;

    vec4 model = rotation_z(model_rotation) * vec4(in_position, 1.0);
    model.xyz += model_position;

    mat3 translation = mat3(
        -cos(model_rotation), sin(model_rotation), 0,
        sin(model_rotation), cos(model_rotation), 0,
        0, 0, 1
    );
    out_normal = in_normal * translation;

    gl_Position = view_proj * model;
    out_position = model.xyz;
    out_uv = in_uv;
    out_model_color = in_model_color;
    out_lighting_color_alpha = vec4(1.0, 1.0, 1.0, 1.0);
}
