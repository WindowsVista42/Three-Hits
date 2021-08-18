#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec4 in_model_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(mix(in_model_color.rgb, texture(tex_sampler, in_tex_coord).rgb, 0.5), in_model_color.a);
    //out_color = in_model_color;
    //out_color = vec4(1.0);
}
