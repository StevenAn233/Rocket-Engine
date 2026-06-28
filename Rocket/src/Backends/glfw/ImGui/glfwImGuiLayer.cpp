module;

#include <ImGuizmo.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

module ImGuiLayer;

import Log;
import String;
import FileUtils;
import Style;
import Window;
import ConfigProxy;

namespace {
    using namespace rke;
    static inline ImGuiIO& io() { return ImGui::GetIO(); }
    static String s_imgui_ini_path{};
}

namespace rke
{
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
        valid_ = true;

    // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io().ConfigFlags |= ImGuiConfigFlags_IsSRGB;

        if(s_imgui_ini_path.empty()) {
            Path temp{ file::editor_dir() / u8"settings" / u8"imgui.ini" };
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

        Path config_path{ file::editor_dir() / u8"config" / u8"style.conf" };
        auto reader{ ConfigReader::create(config_path) };
        auto font_data{ reader ? reader->get_child(u8"Font") : nullptr };
        Path font_path{};
        float font_size_base{};
        if(font_data) {
            font_path = file::unify_path(font_data->get_at(u8"Path", String()));
            font_size_base = font_data->get_at(u8"Size", 16.0f);
        }

        
        if(font_path.exists()) {
            float high_res_font_size{ font_size_base * 2.0f };
            ImFont* font{ io().Fonts->AddFontFromFileTTF
                (font_path.string().raw(), high_res_font_size) };
            CORE_ASSERT(font, u8"glfwImGuiLayer: Fail to load font!");
        } else {
            CORE_WARN(u8"glfwImGuiLayer: Font path '{}' not found!"
                u8" Using default one.", font_path);
        }

        ImGui::GetStyle().FontSizeBase = font_size_base;

    // Setup DPI scaling
        io().ConfigDpiScaleFonts = true;
        io().ConfigDpiScaleViewports = true;

    // Setup platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(std::bit_cast<GLFWwindow*>
            (reinterpret_cast<Window*>(owner_)->get_native_window().get()), true);
        ImGui_ImplOpenGL3_Init("#version 430 core");
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

    void ImGuiLayer::begin_render() const
    {
        if(!valid_) return;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::end_render() const
    {
        if(!valid_) return;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Issue: glBindSample(...) called here ^^^, which doesn't support mipmap;
        // Causing RocketLauncher::Toolbar icons rendered terribly.

        if(io().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* cached{ std::bit_cast<GLFWwindow*>
                (reinterpret_cast<Window*>(owner_)->get_native_window().get()) };
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(cached);
        }
    }
}
