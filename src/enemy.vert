#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view_proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_model_position;
layout(location = 4) in float in_model_rotation_z;

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
    gl_Position = view_proj * rotation_z(in_model_rotation_z) * vec4(in_position, 1.0) * vec4(in_model_position, 1.0);
    frag_color = in_color;
    tex_coord = in_tex_coord;
}
