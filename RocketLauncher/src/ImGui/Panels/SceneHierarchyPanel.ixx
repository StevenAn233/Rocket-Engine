module;
export module SceneHierarchyPanel;

import rke;
import Layout;

export namespace rke
{
    class SceneHierarchyPanel : public Panel
    {
    public:
        using EntityNodeCallback = std::function<void(Scene*)>;

        SceneHierarchyPanel(String name) : Panel(std::move(name)) {}

        void set_context(Scene* context)
        {
            context_ = context;
            is_scene_selected_ = false;
        }

        void set_on_entity_node_render(EntityNodeCallback callback)
            { on_entity_node_render_ = std::move(callback); }

        void on_imgui_render() override;

        template<typename Component, typename Callback>
        requires std::invocable<Callback, Entity>
        void check_then_draw(Entity entity, StringView name, Callback&& callback)
        {
            if(!entity.has<Component>()) return;
            constexpr auto type_id{ entt::type_hash<Component>::value() };

            bool to_delete{ false };
            layout::tree_node_branch(name, [&]()
            {
                if(!ImGui::BeginPopupContextItem()) goto invoking;
                
                if constexpr(std::is_same_v<Component, TagComponent>)
                    { ImGui::CloseCurrentPopup(); }
                else if constexpr(std::is_same_v<Component, UUIDComponent>)
                    { ImGui::CloseCurrentPopup(); }
                else if constexpr(std::is_same_v<Component, TransformComponent>)
                {
                    auto& tc{ entity.get_mut<TransformComponent>() };
                    if(tc.locked) {
                        if(ImGui::MenuItem("Unlock")) {
                            tc.locked = false;
                            context_->mark_modified();
                        }
                    } else {
                        if(ImGui::MenuItem("Lock")) {
                            tc.locked = true;
                            context_->mark_modified();
                        }
                    }
                }
                else if constexpr(std::is_same_v<Component, CameraComponent>)
                {
                    if(ImGui::MenuItem("Make Master"))
                        context_->set_camera_master(entity);
                    if(ImGui::MenuItem("Delete"))
                        to_delete = true;
                }
                else if constexpr(std::is_same_v<Component, SpriteComponent>)
                {
                    if(!entity.has<Rigidbody2DComponent>())
                        { if(ImGui::MenuItem("Delete")) to_delete = true; }
                    else ImGui::CloseCurrentPopup();
                }
                else if constexpr(std::is_same_v<Component, Rigidbody2DComponent>)
                {
                    if(!entity.has<BoxCollider2DComponent>())
                        { if(ImGui::MenuItem("Delete")) to_delete = true; }
                    else ImGui::CloseCurrentPopup();
                }
                else { if(ImGui::MenuItem("Delete")) to_delete = true; }
                ImGui::EndPopup();

            invoking:
                std::invoke(std::forward<Callback>(callback), entity);
            }, 0, std::bit_cast<void*>(static_cast<uint64>(type_id)));

            if(to_delete) {
                entity.remove<Component>();
                if constexpr(std::is_same_v<Component, NativeScriptComponent>)
                    ScriptEngine::update_script(entity);
            }
        }
    private:
        void draw_entity_node(Entity entity, Entity selected);
        void draw_entity_popup();

        void draw_scene_settings();
        void draw_components(Entity selected);
        void add_components_popup(Entity selected);
    private:
        Scene* context_{};

        bool is_scene_selected_{ false };
        EntityNodeCallback on_entity_node_render_{};
    };
}
