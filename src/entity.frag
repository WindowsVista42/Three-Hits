#version 450

layout(set = 0, binding = 1) uniform sampler2D tex_sampler;

struct Light {
    vec4 position_falloff;
    vec4 color_alpha;
};

layout(set = 0, binding = 2, std140) readonly buffer Lights {
    uint light_count;
    Light lights[];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_model_color;
layout(location = 4) in vec4 in_lighting_color_alpha;

layout(location = 0) out vec4 out_color;

float distance_step(float a, float x_2) {
    //return 1.0/(((light_falloff * light_distance) * (light_falloff * light_distance)) + 1.0);
    return 1.0/((a * x_2) + 1.0);
}

float distance_step_smooth(float inv_midpoint_2, float position_2, float midpoint_slope) {
    return 1.0 / (pow(inv_midpoint_2 * position_2, 2.0 * inv_midpoint_2 * midpoint_slope) + 1.0);
}

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    if(in_model_color.w == 0.0) {
        discard;
    }

    vec4 mixed_color = vec4(mix(in_model_color.rgb, texture(tex_sampler, in_uv).rgb, 0.5), in_model_color.a);

    vec4 combined_color_alpha = vec4(0.0);
    for(uint index = 0; index < light_count; index += 1) {
        vec3 light_position = lights[index].position_falloff.xyz;
        float light_falloff = lights[index].position_falloff.w;
        vec4 light_color_alpha = lights[index].color_alpha;

        vec3 diff = light_position - in_position;
        vec3 light_direction = normalize(diff);
        float light_distance_squared = dot(diff, diff);//TODO(sean): use squared distance
        float kdot = dot(in_normal, light_direction);
        kdot = (kdot + 1.0) / 2.0;

        // light distance equation 1/(A*x)^2+1
        //float kdist = 1.0/(((light_falloff * light_distance) * (light_falloff * light_distance)) + 1.0);
        float kdist = distance_step(light_falloff, light_distance_squared);

        combined_color_alpha += vec4(vec3(kdot * kdist) * light_color_alpha.xyz, 0.0);
    }

    float luminance = dot(combined_color_alpha, combined_color_alpha);
    vec4 test = combined_color_alpha;
    test.w = 1.0;
    vec4 c = test * texture(tex_sampler, in_uv);
    out_color = vec4(aces(c.xyz), 1.0);
}
