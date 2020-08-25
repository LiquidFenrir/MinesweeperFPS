#pragma once

#include <GLFW/glfw3.h>

namespace Focus {
    inline bool is_focused = true;
    inline bool was_captured = false;
    inline auto callback = [](GLFWwindow* window, int focused_arg) -> void {
        if(!focused_arg) {
            was_captured = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            if(was_captured) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        is_focused = focused_arg;
    };
}
