#include "GuiMain.h"
#include <iostream>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

int GuiMain(void (*drawFunction)(void*), void* objPtr) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Create a fullscreen window
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Blockbuster", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader: " << glewGetErrorString(glewInitResult) << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true) || !ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "Failed to initialize ImGui" << std::endl;
        return -1;
    }
    ApplyCustomStyle();
    try {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Call the draw function
        try {
            drawFunction(objPtr);
        }
        catch (const std::length_error& e) {
            std::cerr << "std::length_error caught in draw function: " << e.what() << std::endl;
            // Optionally break the loop or handle the error
        }
        catch (const std::exception& e) {
            std::cerr << "Exception caught in draw function: " << e.what() << std::endl;
            // Optionally break the loop or handle the error
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }

        glfwSwapBuffers(window);
    }
    }

catch (const std::exception& e) {
    std::cerr << "Exception caught in main loop: " << e.what() << std::endl;
}

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void ApplyCustomStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f); // Dark background
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Transparent button
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f); // Hovered button
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f); // Active button
}
