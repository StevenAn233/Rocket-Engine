module;

#include <ImGuizmo.h>

module ImGuiSetup;

import Log;
import String;
import FileUtils;
import ImGuiStyle;
import ConfigProxy;

namespace {
    using namespace rke;
    static inline ImGuiIO& io() { return ImGui::GetIO(); }
    static inline String get_imgui_ini_path()
        { return (file::editor_dir() / u8"settings" / u8"imgui.ini").string(); }
}

namespace rke::imgui
{
    void init(NativeWindow context)
    {
    // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io().ConfigFlags |= ImGuiConfigFlags_IsSRGB;
        
    // Setup style
        io().IniFilename = nullptr;
        ImGui::LoadIniSettingsFromDisk(get_imgui_ini_path().raw());

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
        internal::init_window(context);
        internal::init_graphics();
    }

    void shutdown()
    {
        internal::shutdown_graphics();
        internal::shutdown_window();
        ImGui::SaveIniSettingsToDisk(get_imgui_ini_path().raw());
        ImGui::DestroyContext();
    }

    void begin_render()
    {
        internal::begin_render_graphics();
        internal::begin_render_window();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void end_render()
    {
        ImGui::Render();
        internal::end_render_graphics();
        internal::end_render_window();
    }
}
