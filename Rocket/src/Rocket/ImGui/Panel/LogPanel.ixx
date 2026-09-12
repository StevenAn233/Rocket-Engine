module;

#include <array>
#include <vector>
#include <utility>
#include "rke_macros.h"

export module LogPanel;

import Log;
import Panel;
import Path;
import String;
import Types;

export namespace rke
{
    class LogPanel : public Panel
    {
    public:
        LogPanel(String name) : Panel(std::move(name)) {}
        ~LogPanel() override;
        
        void load_from(Path filepath);
    private:
        RKE_API void on_imgui_render() override; // callback
        
        // copies whatever the history gained since last frame
        void pull_new_entries(); 

        void rebuild_visible();
        void measure_heights();
        void rebuild_offsets();

        void draw_toolbar();
        void draw_entries();
        void draw_context_popup();
        float draw_entry(int row);

        void clear_entries();
    private:
        struct Filters // don't serialize
        {
            // Info/Warn/Error/Trace/Critical
            std::array<bool, 5> levels{ true, true, true, true, true };
            bool core{ true };
            bool client{ true };
        };

        struct Settings // serialize
        {
            bool auto_scroll{ true };
            bool wrap{ true };
            bool show_source{ true };
        };

        Filters filters_{};
        Settings settings_{};
        char search_buffer_[128]{};

        std::vector<LogEntry> entries_{}; // copy of the log_history
        uint64 consumed_{ 0 }; // sequence already copied from the history

    // split to two vectors; more cache friendly
        std::vector<String> formatted_{}; // per entry
        std::vector<float > formatted_heights_{}; // per entry, same order

        std::vector<Size > visible_{}; // indices of element : entires(<= entires_.size)
        std::vector<float> row_offsets_{}; // prefix sums over visible

        std::array<Size, 5> level_counts_{};
        float measured_width_{ -1.0f };  // width the heights were measured at
        bool  measured_wrapped_{ true }; // 'wrap' state they were measured with
        bool  formatted_with_source_{ true };
        bool  visible_dirty_{ true };
        bool  scroll_to_bottom_{ false };

        int context_row_{ -1 }; // row the open context popup belongs to
        Path filepath_{};
    };
}
