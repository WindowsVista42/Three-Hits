#version 450

layout(set = 0, binding = 0) readonly uniform Count {
    uint count;
};

layout(push_constant) uniform Aspect {
    float aspect;
};

layout(location = 0) in vec2 in_vertex;
layout(location = 1) in vec2 in_offset;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = in_color;

    vec2 vertex_position = in_vertex;
    vertex_position.y *= aspect;
    // flip the triangle so its backface culled
    if(gl_InstanceIndex >= count) {
        vertex_position.y *= -1.0;
    }
    gl_Position = vec4(vertex_position + in_offset, 0.0, 1.0);
}