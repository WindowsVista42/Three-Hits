#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view_proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec4 in_model_position_rotation;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 tex_coord;

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

    gl_Position = view_proj * model;
    frag_color = in_color;
    tex_coord = in_tex_coord;
}
