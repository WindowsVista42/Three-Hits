#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view_proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 tex_coord;

void main() {
    gl_Position = view_proj * vec4(in_position, 1.0);
    frag_color = in_color;
    tex_coord = in_tex_coord;
}
