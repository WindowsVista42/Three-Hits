#version 450

vec2 crosshair[3] = vec2[](
    vec2( 0.0, -0.04330127/4.0),
    vec2( 0.01, 0.04330127/4.0),
    vec2(-0.01, 0.04330127/4.0)
);

layout(location = 0) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = in_color;
    gl_Position = vec4(crosshair[gl_VertexIndex], 0.0, 1.0);
}
