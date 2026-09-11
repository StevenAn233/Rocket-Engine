module;

#include <array>
#include <vector>
#include <utility>
#include "rke_macros.h"

export module LogPanel;

import Panel;
import String;
import Types;
import Log;

export namespace rke
{
    class LogPanel : public Panel
    {
    public:
        LogPanel(String name) : Panel(std::move(name)) {}
    private:
        RKE_API void on_imgui_render() override;

        void pull_new_entries(); // copies whatever the history gained since last frame
        void rebuild_visible();  // applies filters, formats new rows
        void draw_toolbar();
        void draw_entries();
        void draw_context_popup(); // right-click: row actions + settings
        float draw_entry(int row); // returns the height the row actually took

        void rebuild_layout(float content_width, float line_height);
        void rebuild_offsets();

        void clear_entries();
    private:
        struct Filters // don't serialize
        {
            String search{};
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
        std::vector<String> display_{};   // pre-formatted line per entry
        std::vector<Size> visible_{};     // indices into entries_ that pass the filters
        std::array<Size, 5> level_counts_{};

        std::vector<float> row_heights_{}; // per visible row
        std::vector<float> row_offsets_{}; // prefix sums, size == rows + 1
        float measured_width_{ -1.0f };    // width the heights were measured at
        bool layout_valid_{ false };
        bool layout_dirty_{ false };

        uint64 consumed_{ 0 }; // sequence already copied from the history
        Size formatted_{ 0 };  // entries_[0, formatted_) already have display_ text
        bool formatted_with_source_{ true };
        bool dirty_{ true };
        bool scroll_to_bottom_{ false };

        int hovered_row_{ -1 }; // row under the cursor this frame
        int context_row_{ -1 }; // row the open context popup belongs to
    };
}
