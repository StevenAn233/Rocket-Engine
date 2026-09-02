module;
export module SceneHierarchyPanel;

import rke;

export namespace rke
{
    class SceneHierarchyPanel : public Panel
    {
    public:
        using EntityNodeCallback = std::function<void(Scene*)>;

        SceneHierarchyPanel(String name);

        inline void set_context(Scene* context)
            { context_ = context; is_scene_selected_ = false; }
        inline void set_on_entity_node_render(EntityNodeCallback callback)
            { on_entity_node_render_ = std::move(callback); }

        template<typename Component, StringLiteral Str, typename Callback>
        requires std::invocable<Callback, Entity>
        inline void check_then_draw(Entity entity, Callback&& callback)
        {
            if(!entity.has<Component>()) return;
            constexpr auto type_id{ entt::type_hash<Component>::value() };

            bool to_delete{ false };
            layout::tree_node_branch<Str>([&]()
            {
                if(ImGui::BeginPopupContextItem())
                {
                    if constexpr(std::is_same_v<Component, IdentityComponent>)
                        { ImGui::CloseCurrentPopup(); }
                    else if constexpr(std::is_same_v<Component, TransformComponent>)
                        { transform_comp_popup_content(entity, to_delete); }
                    else if constexpr(std::is_same_v<Component, CameraComponent>)
                        { camera_comp_popup_content(entity, to_delete); }
                    else if constexpr(std::is_same_v<Component, SpriteComponent>)
                        { sprite_comp_popup_content(entity, to_delete); }
                    else { general_comp_popup_content(to_delete); }
                    ImGui::EndPopup();
                }
                std::invoke(std::forward<Callback>(callback), entity);
            }, 0, std::bit_cast<void*>(static_cast<uint64>(type_id)));
            if(to_delete) entity.remove<Component>();
        }
    private:
        void on_imgui_render() override;

        void draw_entity_node(Entity entity, Entity selected);
        void draw_entity_popup();

        void draw_scene_settings();
        void draw_components(Entity selected);
        void add_components_popup(Entity selected);

        void general_comp_popup_content(bool& to_delete);
        void transform_comp_popup_content(Entity entity, bool& to_delete);
        void camera_comp_popup_content(Entity entity, bool& to_delete);
        void sprite_comp_popup_content(Entity entity, bool& to_delete);
    private:
        Scene* context_{};
        bool is_scene_selected_{ false };
        EntityNodeCallback on_entity_node_render_{};
    };
}
