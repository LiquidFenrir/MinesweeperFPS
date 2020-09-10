#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#if GLFW_VERSION_MAJOR < 3
#error "Need GFLW 3.2+"
#endif

