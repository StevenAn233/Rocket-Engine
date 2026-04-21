module;
module SceneSerializer;

import Types;
import Log;
import Components;
import FileUtils;

namespace {
    using namespace rke;

    static SceneSerializer::SerializeHook   s_serialize_hook  {};
    static SceneSerializer::DeserializeHook s_deserialize_hook{};
    // There is (mostly)only one editor in the App;
    // No matter which scene have we loaded, we are gonna have the same editor;
    // If you really wanna change the editor, just call the setters, fine I believe.

    static void serialize_entity(ConfigWriter* writer, const Entity& entity)
    {
        CORE_ASSERT(entity.valid(), u8"SceneSerializer: Entity invalid!");
        CORE_ASSERT(writer, u8"SceneSerializer: Writer null!");
        writer->begin_map();
        writer->write(u8"Entity", ConfigValue(entity.get_uuid().value()));
        
        if(entity.has<TagComponent>()) {
            writer->begin_map(u8"Tag Component");
            const auto& tag_com{ entity.get<TagComponent>() };
            writer->write(u8"Tag", ConfigValue(tag_com.tag));
            writer->end_map();
        }
        if(entity.has<TransformComponent>()) {
            writer->begin_map(u8"Transform Component");
            const auto& tc{ entity.get<TransformComponent>() };
            writer->write(u8"Position", ConfigValue(tc.position));
            writer->write(u8"Rotation", ConfigValue(tc.rotation));
            writer->write(u8"Size"    , ConfigValue(tc.size    ));
            writer->write(u8"Locked"  , ConfigValue(tc.locked  ));
            writer->end_map();
        }
        if(entity.has<CameraComponent>()) {
            writer->begin_map(u8"Camera Component");

            const auto& camera_com{ entity.get<CameraComponent>() };
            const auto& cam{ camera_com.camera };
            writer->write(u8"Projection Type"  , ConfigValue(cam.get_current_type_int        ()));
            writer->write(u8"Perspective FOV"  , ConfigValue(cam.get_perspective_vertical_fov()));
            writer->write(u8"Perspective Far"  , ConfigValue(cam.get_perspective_far_clip    ()));
            writer->write(u8"Perspective Near" , ConfigValue(cam.get_perspective_near_clip   ()));
            writer->write(u8"Orthographic Size", ConfigValue(cam.get_orthographic_size       ()));
            writer->write(u8"Orthographic Far" , ConfigValue(cam.get_orthographic_far_clip   ()));
            writer->write(u8"Orthographic Near", ConfigValue(cam.get_orthographic_near_clip  ()));

           writer->write(u8"Master", ConfigValue(camera_com.master));
           writer->write(u8"Fixed Aspect-Ratio", ConfigValue(camera_com.aspect_ratio_fixed));

            writer->end_map();
        }
        if(entity.has<SpriteComponent>()) {
            writer->begin_map(u8"Sprite Component");

            const auto& sc{ entity.get<SpriteComponent>() };
            writer->write(u8"Color"  , ConfigValue(sc.color));
            writer->write(u8"Texture", ConfigValue(sc.texture.uuid.value()));
            if(sc.texture.has_asset()) {
                writer->begin_map(u8"Settings");
                writer->write(u8"Tiling Factor", ConfigValue(sc.texture.tiling_factor));
                writer->write(u8"Cell Pixels"  , ConfigValue(sc.texture.cell_pixels  ));
                writer->write(u8"Cell Coords"  , ConfigValue(sc.texture.cell_coords  ));
                writer->write(u8"Cell Counts"  , ConfigValue(sc.texture.cell_counts  ));
                writer->end_map();
            }
            writer->write(u8"Blending Mode", static_cast<uint32>(sc.blending_mode));
            writer->write(u8"Rendering Layer", ConfigValue(sc.rendering_layer));

            writer->end_map();
        }
        if(entity.has<Rigidbody2DComponent>()) {
             writer->begin_map(u8"Rigidbody 2D Component");
             const auto& rbc{ entity.get<Rigidbody2DComponent>() };
             writer->write(u8"Type", static_cast<uint32>(rbc.type));
             writer->write(u8"Rotation Fixed", ConfigValue(rbc.rotation_fixed));
             writer->end_map();
        }
        if(entity.has<BoxCollider2DComponent>()) {
             writer->begin_map(u8"Box Collider 2D Component");

             const auto& bcc{ entity.get<BoxCollider2DComponent>() };
             writer->write(u8"Physics Layer", static_cast<uint32>(bcc.layer_index));
             writer->write(u8"Offset"       , ConfigValue(bcc.offset     ));
             writer->write(u8"Size"         , ConfigValue(bcc.size       ));
             writer->write(u8"Density"      , ConfigValue(bcc.density    ));
             writer->write(u8"Friction"     , ConfigValue(bcc.friction   ));
             writer->write(u8"Restitution"  , ConfigValue(bcc.restitution));

             writer->end_map();
        }
        if(entity.has<NativeScriptComponent>()) {
            writer->begin_map(u8"Native-Script Component");
            const auto& nsc{ entity.get<NativeScriptComponent>() };
            writer->write(u8"Script Name"    , ConfigValue(nsc.script_name    ));
            writer->write(u8"Wants to Update", ConfigValue(nsc.wants_to_update));
            writer->end_map();
        }

        writer->end_map();
    }

    static void deserialize_entity(const ConfigReader* reader, Scene* scene)
    {
        CORE_ASSERT(scene, u8"SceneSerializer: Scene null!");
        CORE_ASSERT(reader, u8"SceneSerializer: Reader null!");

        auto name_config{ reader->get_child(u8"Tag Component") };
        String name{ name_config->get_at(u8"Tag", String{}) };
        UUID id{ reader->get_at(u8"Entity", 0ui64) };
        Entity entity{ scene->create_entity(name, id) };

        Scope<ConfigReader> tc_reader{ reader->get_child(u8"Transform Component") };
        if(tc_reader) {
            auto& tc{ entity.get_mut<TransformComponent>() };
            tc.position = tc_reader->get_at(u8"Position", tc.position);
            tc.rotation = tc_reader->get_at(u8"Rotation", tc.rotation);
            tc.size     = tc_reader->get_at(u8"Size"    , tc.size    );
            tc.locked   = tc_reader->get_at(u8"Locked"  , tc.locked  );
        }

        Scope<ConfigReader> cc_reader{ reader->get_child(u8"Camera Component") };
        if(cc_reader) {
            auto& cc{ entity.emplace<CameraComponent>() };
            cc.master = cc_reader->get_at(u8"Master", false);
            cc.aspect_ratio_fixed = cc_reader->get_at(u8"Fixed Aspect-Ratio", false);

            cc.camera.set_current_type(cc_reader->get_at(u8"Projection Type", 0));
            cc.camera.set_perspective (
                cc_reader->get_at(u8"Perspective FOV" , glm::radians(45.0f)),
                cc_reader->get_at(u8"Perspective Near", 0.01f ),
                cc_reader->get_at(u8"Perspective Far" , 100.0f));
            cc.camera.set_orthographic (
                cc_reader->get_at(u8"Orthographic Size",  10.0f),
                cc_reader->get_at(u8"Orthographic Near", -10.0f),
                cc_reader->get_at(u8"Orthographic Far" ,  10.0f));
        }

        Scope<ConfigReader> sc_reader{ reader->get_child(u8"Sprite Component") };
        if(sc_reader) {
            auto& sc{ entity.emplace<SpriteComponent>
                (AssetUUID(sc_reader->get_at(u8"Texture", 0ui64)))};
            Scope<ConfigReader> tex_config{ sc_reader->get_child(u8"Settings") };
            if(sc.texture.has_asset() && tex_config) {
                sc.texture.tiling_factor = tex_config->get_at(u8"Tiling Factor", 1.0f);
                sc.texture.cell_pixels   = tex_config->get_at(u8"Cell Pixels", glm::vec2(1.0f));
                sc.texture.cell_coords   = tex_config->get_at(u8"Cell Coords", glm::vec2(0.0f));
                sc.texture.cell_counts   = tex_config->get_at(u8"Cell Counts", glm::vec2(1.0f));
            }
            sc.color = sc_reader->get_at(u8"Color", glm::vec4(1.0f));
            sc.blending_mode = static_cast<SpriteComponent::BlendingMode>
                (sc_reader->get_at(u8"Blending Mode", 0ui32));
            sc.rendering_layer = sc_reader->get_at(u8"Rendering Layer", 0);
        }

        Scope<ConfigReader> rbc_reader{ reader->get_child(u8"Rigidbody 2D Component") };
        if(rbc_reader) {
            auto& rbc{ entity.emplace<Rigidbody2DComponent>() };
            rbc.type = static_cast<Rigidbody2DComponent::BodyType>
                (rbc_reader->get_at(u8"Type", 0ui32));
            rbc.rotation_fixed = rbc_reader->get_at(u8"Rotation Fixed", false);
        }

        Scope<ConfigReader> bcc_reader{ reader->get_child(u8"Box Collider 2D Component") };
        if(bcc_reader) {
            auto& bcc{ entity.emplace<BoxCollider2DComponent>() };
            bcc.layer_index = bcc_reader->get_at(u8"Physics Layer", 0ui32);
            bcc.offset      = bcc_reader->get_at(u8"Offset", glm::vec2(0.0f));
            bcc.size        = bcc_reader->get_at(u8"Size"  , glm::vec2(0.5f));
            bcc.density	    = bcc_reader->get_at(u8"Density"    , 1.0f);
            bcc.friction    = bcc_reader->get_at(u8"Friction"   , 0.5f);
            bcc.restitution = bcc_reader->get_at(u8"Restitution", 0.0f);
        }

        Scope<ConfigReader> nsc_reader{ reader->get_child(u8"Native-Script Component") };
        if(nsc_reader) {
            auto& nsc{ entity.emplace<NativeScriptComponent>() };
            nsc.script_name     = nsc_reader->get_at(u8"Script Name", String{});
            nsc.wants_to_update = nsc_reader->get_at(u8"Wants to Update", true);
        }
    }
}

namespace rke
{
    SceneSerializer::SceneSerializer(Ref<Scene> scene) : scene_(std::move(scene))
        { CORE_ASSERT(scene_, u8"SceneSerializer: Scene empty!"); }

    bool SceneSerializer::serialize(const Path& filepath)
    {
        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Scene"  , scene_->get_name   ());
        writer->write(u8"Gravity", scene_->get_gravity());

        writer->begin_array(u8"Entities");
        auto view{ scene_->registry_->view<UUIDComponent>() };
        for(auto it{ view.rbegin() }; it != view.rend(); ++it)
        {
            Entity entity( scene_->get_entity(*it) );
            if(!entity.valid()) continue;
            serialize_entity(writer.get(), entity);
        }
        writer->end_array();

        Entity selected{ scene_->get_selected_entity() };
        if(selected.valid()) writer->write
            (u8"Selected Entity", selected.get_uuid().value());
        else if(!selected.empty()) CORE_ERROR
            (u8"SceneSerializer: Selected entity invalid!");

        if(s_serialize_hook) s_serialize_hook(this, writer.get());

        writer->end_map();

        file::check_to_create_dir(filepath);
        if(writer->push_to_file(filepath))
            { scene_->modified_ = false; return true; }

        CORE_ERROR(u8"SceneSerializer: Failed to serialize scene '{}'!", filepath);
        return false;
    }

    bool SceneSerializer::deserialize(const Path& filepath)
    {
        if(!filepath.exists()) {
            CORE_ERROR(u8"SceneSerializer: File '{}' doesn't exist!", filepath);
            return false;
        }
        if(!filepath.string().ends_with(u8".rkscene")) {
            CORE_ERROR(u8"SceneSerializer: '{}' isn't a .rkscene file!", filepath);
            return false;
        }

        Scope<ConfigReader> reader{ ConfigReader::create(filepath) };
        scene_->set_name(reader->get_at(u8"Scene", String{}));
        scene_->get_gravity_mut() = reader->get_at(u8"Gravity", Gravity2D::get_default());

        Scope<ConfigReader> entities{ reader->get_child(u8"Entities") };
        if(!entities) {
            CORE_WARN(u8"SceneSerializer: No entities found in file '{}'!", filepath);
            return false;
        }
        if(!entities->is_array()) {
            CORE_WARN(u8"SceneSerializer: File format incorrect!");
            return false;
        }
        // When you traverse a Sequence, each time you get a Node.
        // When you traverse a Map, each time you get a pair<Node, Node>.
        entities->for_each_arr([this](Scope<ConfigReader> entity)
            { deserialize_entity(entity.get(), scene_.get()); });

        if(reader->has_key(u8"Selected Entity")) {
            UUID selected_id{reader->get_at(u8"Selected Entity", 0ui64)};
            scene_->set_selected_entity(selected_id);
        }
        else scene_->set_selected_entity(Entity{});

        if(s_deserialize_hook) s_deserialize_hook(this, scene_.get(), reader.get());

        scene_->get_master_camera();
        scene_->modified_ = false; // just loaded, nothing changed
        return true;
    }

    void SceneSerializer::set_serialize_hook(SerializeHook hook)
        { s_serialize_hook = std::move(hook); }
    void SceneSerializer::set_deserialize_hook(DeserializeHook hook)
        { s_deserialize_hook = std::move(hook); }
}
