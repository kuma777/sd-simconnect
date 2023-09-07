#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <Windows.h>

#include "logger.hpp"
#include "simconnect.hpp"
#include "streamdeck.hpp"

int HandleError()
{
    LOG_EXPORT_END();
    return -1;
}

int
main(int argc, const char* argv[])
{
    LOG_EXPORT_BEGIN("./simconnect.log");

    int port = 0;
    std::string pluginUUID;
    std::string registerEvent;
    std::string info;

    for (int i = 1; i < argc; ++i)
    {
        std::string key(argv[i]);
        if (key == "-port")
        {
            port = std::atoi(argv[i + 1]);
        }
        else if (key == "-pluginUUID")
        {
            pluginUUID.assign(argv[i + 1]);
        }
        else if (key == "-registerEvent")
        {
            registerEvent.assign(argv[i + 1]);
        }
        else if (key == "-info")
        {
            info.assign(argv[i + 1]);
        }
    }

    if (!glfwInit())
    {
        return HandleError();
    }

    auto window = glfwCreateWindow(800, 600, "sd-simconnect", nullptr, nullptr);
    if (!window)
    {
        return HandleError();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

    StreamDeck deck(port, registerEvent, pluginUUID);
    if (!deck.start())
    {
        LOG_ERROR("Failed to start connection to StreamDeck.");
        return HandleError();
    }

    SimConnect sim;
    if (!sim.start())
    {
        LOG_ERROR("Failed to start SimConnect.");
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

        sim.poll();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#ifndef NDEBUG
        ImGui::Begin("Log");
        {
            ImGui::SetWindowPos({ 10, 10 });
            ImGui::SetWindowSize({ 600, 400 });
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

    deck.finalize();

    LOG_EXPORT_END();

    return 0;
}