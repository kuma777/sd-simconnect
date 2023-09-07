#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <Windows.h>
#include <websocket.h>

#include <SimConnect.h>

#include "logger.hpp"

int WINAPI
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    if (!glfwInit())
        return -1;

    auto window = glfwCreateWindow(800, 600, "sd-mcp", nullptr, nullptr);
    if (!window)
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

    HRESULT result;

    WEB_SOCKET_HANDLE socket;
    result = WebSocketCreateClientHandle(nullptr, 0, &socket);

    if (result != S_OK)
    {
        LOG_ERROR("Failed to create WebSocket client.");
    }

    HANDLE sim;
    result = SimConnect_Open(&sim, "sd-mcp", NULL, 0, 0, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL);

    if (result != S_OK)
    {
        LOG_ERROR("Failed to create SimConnect handle.");
    }

    const ImColor White(ImVec4(1.0, 1.0, 1.0, 1.0));
    const ImColor LightGray(ImVec4(1.0, 1.0, 1.0, 0.8));
    const ImColor DarkGray(ImVec4(1.0, 1.0, 1.0, 0.3));
    const ImColor Black(ImVec4(0.0, 0.0, 0.0, 1.0));
    const ImColor Red(ImVec4(1.0, 0.0, 0.0, 1.0));
    const ImColor Green(ImVec4(0.0, 1.0, 0.0, 1.0));
    const ImColor Yellow(ImVec4(1.0, 1.0, 0.0, 1.0));
    const ImColor Orange(ImVec4(1.0, 0.5, 0.0, 1.0));
    const ImColor DarkGreen(ImVec4(0.0, 1.0, 0.0, 0.5));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Button("Test");

#ifndef NDEBUG
        ImGui::Begin("Log");
        {
            for (auto& message : Logger::Instance().getHistory())
            {
                switch (message.first)
                {
                case 1:
                    ImGui::TextColored(LightGray, message.second.c_str());
                    break;
                case 3:
                    ImGui::TextColored(White, message.second.c_str());
                    break;
                }
            }
        }
        ImGui::End();
#endif

        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1, 0.1, 0.1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}