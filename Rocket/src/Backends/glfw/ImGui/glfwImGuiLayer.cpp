module;

#include <ImGuizmo.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

module ImGuiLayer;

import Log;
import Input;
import Application;
import RenderCommand;
import FileUtils;
import MathUtils;
import Style;

namespace {
    using namespace rke;

    static inline ImGuiIO& io() { return ImGui::GetIO(); }

    static void render_window_callback(ImGuiViewport* viewport, void*)
    {
        if(!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        {
            static constexpr ImVec4 clear_color(0.0f, 0.0f, 0.0f, 1.0f);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        ImGui_ImplOpenGL3_RenderDrawData(viewport->DrawData);
    }

    static ImGuiLayer* instance_{};
    static String s_imgui_ini_path{};
}

namespace rke
{
    ImGuiLayer::ImGuiLayer(String window_title, String name, StyleConfig config)
        : Layer(std::move(window_title), std::move(name))
        , style_config_(std::move(config))
    {
        CORE_ASSERT(!instance_, u8"glfwImGuiLayer: Only one ImGuiLayer supported!");
        instance_ = this;
    }

    void ImGuiLayer::on_event(Event& e)
    {
        if(!valid_) return;
        if(e.is_in_category(EventCategoryMouse))
            e.handled_ |= should_block_mouse();
        if(e.is_in_category(EventCategoryKeyboard))
            e.handled_ |= should_block_keyboard();
    }

    void ImGuiLayer::on_attach()
    {
        Layer::on_attach();
        valid_ = true;

    // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io().ConfigFlags |= ImGuiConfigFlags_IsSRGB;

        if(s_imgui_ini_path.empty()) {
            Path temp{ file::find_editor_dir() / u8"settings" / u8"imgui.ini" };
            CORE_ASSERT(temp.exists(), u8"glfwImGuiLayer: Editor path doesn't exist!");
            s_imgui_ini_path = temp.string();
        }
        io().IniFilename = s_imgui_ini_path.raw();

    // Setup style
        style::imgui_darktheme();

        ImGuiStyle& style{ ImGui::GetStyle() };
        style.CellPadding.y = 0.0f;
        style.ItemSpacing.x = 4.0f;
        style.ItemSpacing.y = 4.0f;
        if(io().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            // Bg for Background
        }

    // Setup fonts
        io().Fonts->Clear();

        float font_size_base{ style_config_.font_size };
        float high_res_font_size{ font_size_base * 2.0f };

        Path font_path{ Application::get().asset_path(style_config_.font_path) };
        CORE_ASSERT(font_path.exists(),
            u8"glfwImGuiLayer: Font path '{}' not found!", font_path);

        ImFont* font{ io().Fonts->AddFontFromFileTTF
            (font_path.string().raw(), high_res_font_size) };
        CORE_ASSERT(font, u8"glfwImGuiLayer: Fail to load font!");

        ImGui::GetStyle().FontSizeBase = font_size_base;

    // Setup DPI scaling
        io().ConfigDpiScaleFonts	 = true;
        io().ConfigDpiScaleViewports = true;

    // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        ImGuiPlatformIO& platform_io{ ImGui::GetPlatformIO() };
        platform_io.Renderer_RenderWindow = render_window_callback;
    }

    void ImGuiLayer::on_detach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        valid_ = false;
    }

    bool ImGuiLayer::should_block_mouse()
    {
        if(!valid_) return false;
        if(main_viewport_hovered_) return false;
        return io().WantCaptureMouse;
    }

    bool ImGuiLayer::should_block_keyboard()
    {
        if(!valid_) return false;
        if(io().WantTextInput) return true;
        if(main_viewport_focused_) return false;
        return io().WantCaptureKeyboard;
    }

    void ImGuiLayer::begin_render()
    {
        if(!valid_) return;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::end_render()
    {
        if(!valid_) return;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if(io().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context{ glfwGetCurrentContext() };
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }
}
