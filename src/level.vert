#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view_proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

void main() {
    out_position = in_position;
    out_normal = in_normal;
    out_uv = in_uv;

    gl_Position = view_proj * vec4(in_position, 1.0);
}
