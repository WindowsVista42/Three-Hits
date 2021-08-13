#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = mix(vec4(in_color, 0.0), texture(tex_sampler, in_tex_coord), 1.0);
    //out_color = vec4(1.0);
}
