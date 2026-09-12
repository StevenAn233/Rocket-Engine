module;
module LogPanel;

import Application;

namespace {
    using namespace rke;

    constexpr StringView level_name_sv(LogLevel level)
    {
        using namespace rke::literals;
        switch(level)
        {
        case LogLevel::Info:     return u8"Info"_sv;
        case LogLevel::Warn:     return u8"Warn"_sv;
        case LogLevel::Error:    return u8"Error"_sv;
        case LogLevel::Trace:    return u8"Trace"_sv;
        case LogLevel::Critical: return u8"Critical"_sv;
        }
        return u8"Unknown"_sv;
    }

    constexpr ImVec4 level_color(LogLevel level)
    {
        switch(level)
        {
        case LogLevel::Info:     return ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
        case LogLevel::Warn:     return ImVec4(0.95f, 0.85f, 0.40f, 1.0f);
        case LogLevel::Error:    return ImVec4(0.95f, 0.45f, 0.45f, 1.0f);
        case LogLevel::Trace:    return ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
        case LogLevel::Critical: return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
        }
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static bool icontains(StringView haystack, StringView needle)
    {
        if(needle.empty()) return true;
        const Size hay_size{ haystack.size() };
        const Size needle_size{ needle.size() };
        if(needle_size > hay_size) return false;

        const char* hay{ haystack.raw_unsafe() };
        const char* pin{ needle.raw_unsafe() };
        const auto lower{ [](char ch) -> char
        {
            return (ch >= 'A' && ch <= 'Z') ?
                static_cast<char>(ch - 'A' + 'a') : ch;
        }};

        for(Size offset{}; offset + needle_size <= hay_size; ++offset)
        {
            bool match{ true };
            for(Size i{}; i < needle_size; ++i)
            {
                if(lower(hay[offset + i]) == lower(pin[i])) continue;
                match = false;
                break;
            }
            if(match) return true;
        }
        return false;
    }

    static String build_display_text(const LogEntry& entry, bool show_source)
    {
        auto zoned_time{ std::chrono::zoned_time(std::chrono::current_zone(), entry.time) };
        if(!show_source) return String::format(u8"[{:%T}] {}", zoned_time, entry.message);

        String file_name{ str::extract_filename
            (StringView(str::to_char8(entry.file ? entry.file : ""))) };
        return String::format(u8"[{:%T}][{}:{}] {}",
            zoned_time, file_name, entry.line, entry.message);
    }

    static void vertical_separator(float height)
    {
        const ImVec2 top{ ImGui::GetCursorScreenPos() };

        ImGui::Dummy(ImVec2(1.0f, height));
        ImGui::GetWindowDrawList()->AddLine(top, ImVec2(top.x, top.y + height),
            ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
    }
}

namespace rke
{
    extern LogHistory log_history;

    void LogPanel::on_imgui_render()
    {
        ImGui::Begin(get_name().raw());

        pull_new_entries();
        if(visible_dirty_) rebuild_visible();

        draw_toolbar();
        draw_entries();
        draw_context_popup();

        ImGui::End();
    }

    void LogPanel::pull_new_entries()
    {
        if(log_history.written() == consumed_) return;
        consumed_ = log_history.copy_since(consumed_, entries_);
        visible_dirty_ = true;
    }

    void LogPanel::rebuild_visible()
    {
        if(formatted_with_source_ != settings_.show_source)
        {
            formatted_with_source_ = settings_.show_source;
            formatted_cnt_ = 0;
        }
        formatted_.resize(entries_.size());
        for(; formatted_cnt_ < formatted_.size(); formatted_cnt_++)
            formatted_[formatted_cnt_] = build_display_text
                (entries_[formatted_cnt_], settings_.show_source);

        visible_.clear();
        level_counts_.fill(0);
        for(Size i{}; i < entries_.size(); ++i)
        {
            const LogEntry& entry{ entries_[i] };
            level_counts_[static_cast<Size>(entry.level)]++;

            if(!filters_.levels[static_cast<Size>(entry.level)]) continue;
            if(entry.type == LogType::Core ? !filters_.core : !filters_.client) continue;
            if(!icontains(entry.message, filters_.search)) continue;

            visible_.push_back(i);
        }
        if(settings_.auto_scroll) scroll_to_bottom_ = true;
        rebuild_layout();
        visible_dirty_ = false;
    }

    void LogPanel::rebuild_layout()
    {
        const Size rows{ visible_.size() };
        const float spacing{ ImGui::GetStyle().ItemSpacing.y };
        const float wrap_width{ measured_width_ > 0.0f ? measured_width_ : 1.0f };

        row_heights_.resize(rows);
        for(Size row{}; row < rows; ++row)
        {
            const String& text{ formatted_[visible_[row]] };
            row_heights_[row] = (settings_.wrap
                ? ImGui::CalcTextSize(text.raw(), nullptr, false, wrap_width).y
                : ImGui::GetTextLineHeight()) + spacing;
        }
        rebuild_offsets();
        layout_valid_ = true;
    }

    void LogPanel::rebuild_offsets()
    {
        row_offsets_.resize(row_heights_.size() + 1);

        float offset{ 0.0f };
        for(Size row{}; row < row_heights_.size(); ++row)
        {
            row_offsets_[row] = offset;
            offset += row_heights_[row];
        }
        row_offsets_[row_heights_.size()] = offset; // exact content height
        layout_dirty_ = false;
    }

    void LogPanel::draw_toolbar()
    {
        const float row_height{ ImGui::GetFrameHeight() };

    // group 1: search
        ImGui::SetNextItemWidth(220.0f);
        if(ImGui::InputTextWithHint("##log_search", "search...",
            search_buffer_, sizeof(search_buffer_)))
        {
            filters_.search = String{ str::to_char8(search_buffer_) };
            visible_dirty_ = true;
        }
        ImGui::SameLine();
        vertical_separator(row_height);

    // group 2: senders
        if(ImGui::Checkbox("Core", &filters_.core)) visible_dirty_ = true;
        ImGui::SameLine(0.0f, 10.0f);
        if(ImGui::Checkbox("Client", &filters_.client)) visible_dirty_ = true;
        ImGui::SameLine();
        vertical_separator(row_height);

    // group 3: level toggles, colored like the rows themselves
        for(Size i{ 0 }; i < filters_.levels.size(); ++i)
        {
            const LogLevel level{ static_cast<LogLevel>(i) };
            char label[48]{};
            std::snprintf(label, sizeof(label), "%s(%zu)",
                level_name_sv(level).raw_unsafe(), static_cast<size_t>(level_counts_[i]));

            ImGui::PushStyleColor(ImGuiCol_Text, level_color(level));
            if(i) ImGui::SameLine();
            if(ImGui::Checkbox(label, &filters_.levels[i])) visible_dirty_ = true;
            ImGui::PopStyleColor();
        }
        
        ImGui::Text("%zu entries, %zu shown", entries_.size(), visible_.size());
        const uint64 recycled{ log_history.dropped() };
        if(recycled > 0)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f),
                "(%llu older entries recycled)", recycled);
        }
    }

    void LogPanel::draw_entries()
    {
        ImGui::BeginChild("##log_entries", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        hovered_row_ = -1; // re-detected while the rows below are submitted

        const float content_width{ ImGui::GetContentRegionAvail().x };
        if(measured_width_ != content_width) 
        {
            measured_width_ = content_width;
            layout_valid_ = false;
        }
        if(!layout_valid_) rebuild_layout();

    // keep following the tail, but never yank the view while the user reads history
        const bool at_bottom{ ImGui::GetScrollY() < (ImGui::GetScrollMaxY() - 1.0f) };

        const Size rows{ visible_.size() };
        const float view_top{ ImGui::GetScrollY() };
        const float view_bottom{ view_top + ImGui::GetWindowHeight() };

    // first row that may be visible: binary search over the prefix sums
        Size first{ 0 };
        {
            Size low{ 0 }, high{ rows };
            while(low < high)
            {
                const Size mid{ low + (high - low) / 2 };
                if(row_offsets_[mid] + row_heights_[mid] < view_top) low = mid + 1;
                else high = mid;
            }
            first = low;
        }

        for(Size row{ first }; row < rows; ++row)
        {
            if(row_offsets_[row] > view_bottom) break; // starts below the viewport

            ImGui::SetCursorPosY(row_offsets_[row]);
            const float real_height{ draw_entry(static_cast<int>(row))
                + ImGui::GetStyle().ItemSpacing.y };

            float delta{ real_height - row_heights_[row] };
            if(delta < 0.0f) delta = -delta;
            if(delta > 0.5f)
            {
                row_heights_[row] = real_height;
                layout_dirty_ = true;
            }
        }
        if(layout_dirty_) rebuild_offsets();

    // anchor the content height, so the scrollbar reaches the very last row
        ImGui::SetCursorPosY(row_offsets_.empty() ? 0.0f : row_offsets_.back());
        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        if(!at_bottom || scroll_to_bottom_) ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom_ = false;

        ImGui::EndChild();
    }

    void LogPanel::draw_context_popup()
    {
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
        && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            context_row_ = hovered_row_;
            ImGui::OpenPopup("##log_context");
        }

        if(!ImGui::BeginPopup("##log_context")) return;

        if(context_row_ >= 0 && static_cast<Size>(context_row_) < visible_.size())
        {
            const Size index{ visible_[static_cast<Size>(context_row_)] };
            if(ImGui::MenuItem("Copy Line"))    ImGui::SetClipboardText(formatted_[index].raw());
            if(ImGui::MenuItem("Copy Message")) ImGui::SetClipboardText(entries_[index].message.raw());
            ImGui::Separator();
        }

        if(ImGui::MenuItem("Copy All"))
        {
            String joined{};
            for(Size index : visible_)
            {
                joined += formatted_[index];
                joined += u8"\n";
            }
            if(!joined.empty()) ImGui::SetClipboardText(joined.raw());
        }
        if(ImGui::MenuItem("Clear")) clear_entries();
        ImGui::Separator();

        ImGui::MenuItem("Auto-scroll", nullptr, &settings_.auto_scroll);
        if(ImGui::MenuItem("Wrap", nullptr, &settings_.wrap)) layout_valid_ = false;
        if(ImGui::MenuItem("Show Source", nullptr, &settings_.show_source)) visible_dirty_ = true;

        ImGui::EndPopup();
    }

    float LogPanel::draw_entry(int row)
    {
        const Size index{ visible_[static_cast<Size>(row)] };
        const String& text{ formatted_[index] };

        ImGui::PushStyleColor(ImGuiCol_Text, level_color(entries_[index].level));
        if(settings_.wrap) ImGui::TextWrapped("%s", text.raw());
        else ImGui::TextUnformatted(text.raw());
        ImGui::PopStyleColor();

        const float height{ ImGui::GetItemRectSize().y };

    // hover must still register while the context popup is open, otherwise the
    // popup could not tell which row it was opened on
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            hovered_row_ = row;

        return height;
    }

    void LogPanel::clear_entries()
    {
        log_history.clear();

        entries_.clear();
        formatted_.clear();
        visible_.clear();
        level_counts_.fill(0);

        consumed_ = log_history.written();
        formatted_cnt_ = 0;
        layout_valid_ = false;
        scroll_to_bottom_ = true;
    }
}
