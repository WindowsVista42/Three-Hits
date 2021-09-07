#version 450

/*
layout(set = 0, binding = 0) readonly uniform UiUniformObject {
    //vec4 color;
    //uint count;
    float aspect;
};
*/

layout(set = 0, binding = 0) readonly uniform Count {
    uint count;
};

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = in_color;
}