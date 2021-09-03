//
// Created by Windows Vista on 9/2/2021.
//

#ifndef THREE_HITS_INPUT_H

typedef struct Bind {
    i32 bind;
    b8 held, pressed, _pad0, _pad1;
} Bind;

Bind new_bind(const int key) {
    Bind bind = {key};
    return bind;
}

void update_key_bind_state(GLFWwindow* window, Bind* restrict bind) {
    if(glfwGetKey(window, bind->bind) == GLFW_PRESS) {
        if(!bind->held) {
            bind->pressed = true;
        } else {
            bind->pressed = false;
        }
        bind->held = true;
    } else {
        bind->held = false;
        bind->pressed = false;
    }
}

void update_mouse_bind_state(GLFWwindow* window, Bind* bind) {
    if(glfwGetMouseButton(window, bind->bind) == GLFW_PRESS) {
        if(!bind->held) {
            bind->pressed = true;
        } else {
            bind->pressed = false;
        }
        bind->held = true;
    } else {
        bind->held = false;
        bind->pressed = false;
    }
}

#define THREE_HITS_INPUT_H

#endif //THREE_HITS_INPUT_H
