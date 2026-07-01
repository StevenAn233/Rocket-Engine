module;

#include <ImGuizmo.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

module ImGuiLayer;

import Log;
import String;
import FileUtils;
import ImGuiStyle;
import Window;
import ConfigProxy;

namespace {
    using namespace rke;
    static inline ImGuiIO& io() { return ImGui::GetIO(); }
}

namespace rke
{
    void ImGuiLayer::on_event(Event& e)
    {
        if(!valid_) return;
        if(e.belongs_to(EventCategoryMouse))
            e.handled_ |= should_block_mouse();
        if(e.belongs_to(EventCategoryKeyboard))
            e.handled_ |= should_block_keyboard();
    }

    void ImGuiLayer::on_attach() { valid_ = true;  }
    void ImGuiLayer::on_detach() { valid_ = false; }

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

    void ImGuiLayer::begin_render() const {}
    void ImGuiLayer::end_render() const {}
}

namespace rke::imgui
{
    static const String& get_imgui_ini_path()
    {
        static const String s_ini_path {
            (file::editor_dir() / u8"settings" / u8"imgui.ini")
        .string() };
        return s_ini_path;
    }

    void init(Window* window)
    {
    // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io().ConfigFlags |= ImGuiConfigFlags_IsSRGB;
        io().IniFilename = get_imgui_ini_path().raw();

    // Setup style
        imgui::style::darktheme();
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
        Path config_path{ file::editor_dir() / u8"config" / u8"style.conf" };
        auto reader{ ConfigReader::create(config_path) };
        auto font_data{ reader ? reader->get_child(u8"Font") : nullptr };
        Path font_path{};
        float font_size_base{};
        if(font_data) {
            font_path = file::unify_path(font_data->get_at(u8"Path", String()));
            font_size_base = font_data->get_at(u8"Size", 16.0f);
        }
        ImGui::GetStyle().FontSizeBase = font_size_base;

        if(font_path.exists()) {
            io().Fonts->Clear();
            float high_res_font_size{ font_size_base * 2.0f };
            ImFont* font{ io().Fonts->AddFontFromFileTTF
                (font_path.string().raw(), high_res_font_size) };
            CORE_ASSERT(font, u8"glfwImGuiLayer: Fail to load font!");
        } else {
            CORE_WARN(u8"glfwImGuiLayer: Font path '{}' not found!"
                u8" Using default one.", font_path);
        }

    // Setup DPI scaling
        io().ConfigDpiScaleFonts = true;
        io().ConfigDpiScaleViewports = true;

    // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(std::bit_cast
            <GLFWwindow*>(window->get_context().get()), true);
        ImGui_ImplOpenGL3_Init("#version 430 core");
    }

    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void begin_render()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void end_render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Issue: glBindSample(...) called here ^^^, which doesn't support mipmap;
        // Causing RocketLauncher::Toolbar icons rendered terribly.

        if(io().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* cached{ glfwGetCurrentContext() };
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(cached);
        }
    }
}
