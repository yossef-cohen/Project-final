#ifndef GUIMAIN_H
#define GUIMAIN_H

#include <../GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

void ApplyCustomStyle();
int GuiMain(void (*drawfunction)(void*), void* obj_ptr);

#endif // GUIMAIN_H
