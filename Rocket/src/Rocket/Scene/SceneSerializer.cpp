module;
module SceneSerializer;

import Types;
import Log;
import Components;
import FileUtils;

namespace {
    using namespace rke;

    static void serialize_entity(ConfigWriter& writer, Entity entity)
    {
        CORE_ASSERT(entity.valid(), u8"SceneSerializer: Entity invalid!");
        writer.begin_map();

        auto& id_com{ entity.get<IdentityComponent>() };
        writer.write(u8"Entity", ConfigValue(id_com.uuid.value()));
        writer.write(u8"Tag", ConfigValue(id_com.tag));
        
        if(entity.has<TransformComponent>())
        {
            writer.begin_map(u8"Transform Component");

            const auto& tc{ entity.get<TransformComponent>() };
            writer.write(u8"Position", ConfigValue(tc.position));
            writer.write(u8"Rotation", ConfigValue(tc.rotation));
            writer.write(u8"Size", ConfigValue(tc.size));
            writer.write(u8"Locked", ConfigValue(tc.locked));

            writer.end_map();
        }
        if(entity.has<CameraComponent>())
        {
            writer.begin_map(u8"Camera Component");

            const auto& camera_com{ entity.get<CameraComponent>() };
            const auto& cam{ camera_com.camera };
            writer.write(u8"Projection Type"  , ConfigValue(cam.get_current_type_int        ()));
            writer.write(u8"Perspective FOV"  , ConfigValue(cam.get_perspective_vertical_fov()));
            writer.write(u8"Perspective Far"  , ConfigValue(cam.get_perspective_far_clip    ()));
            writer.write(u8"Perspective Near" , ConfigValue(cam.get_perspective_near_clip   ()));
            writer.write(u8"Orthographic Size", ConfigValue(cam.get_orthographic_size       ()));
            writer.write(u8"Orthographic Far" , ConfigValue(cam.get_orthographic_far_clip   ()));
            writer.write(u8"Orthographic Near", ConfigValue(cam.get_orthographic_near_clip  ()));

            writer.write(u8"Fixed Aspect-Ratio", ConfigValue(camera_com.aspect_ratio_fixed));

            writer.end_map();
        }
        if(entity.has<SpriteComponent>())
        {
            writer.begin_map(u8"Sprite Component");

            const auto& sc{ entity.get<SpriteComponent>() };
            writer.write(u8"Color"  , ConfigValue(sc.color));
            writer.write(u8"Texture", ConfigValue(sc.sprite.tex_uuid.value()));
            if(sc.sprite.has_texture())
            {
                writer.begin_map(u8"Settings");
                writer.write(u8"Tiling Factor", ConfigValue(sc.sprite.tiling_factor));
                writer.write(u8"Cell Pixels", ConfigValue(sc.sprite.cell_pixels));
                writer.write(u8"Cell Coords", ConfigValue(sc.sprite.cell_coords));
                writer.write(u8"Cell Counts", ConfigValue(sc.sprite.cell_counts));
                writer.end_map();
            }
            writer.write(u8"Blending Mode", static_cast<uint32>(sc.blending_mode));
            writer.write(u8"Rendering Layer", ConfigValue(sc.rendering_layer));

            writer.end_map();
        }
        if(entity.has<Rigidbody2DComponent>())
        {
            writer.begin_map(u8"Rigidbody 2D Component");

            const auto& rbc{ entity.get<Rigidbody2DComponent>() };
            writer.write(u8"Type", static_cast<uint32>(rbc.type));
            writer.write(u8"Rotation Fixed", ConfigValue(rbc.rotation_fixed));

            writer.end_map();
        }
        if(entity.has<BoxCollider2DComponent>())
        {
            writer.begin_map(u8"Box Collider 2D Component");

            const auto& bcc{ entity.get<BoxCollider2DComponent>() };
            writer.write(u8"Collider Type", static_cast<uint32>(bcc.type));
            writer.write(u8"Physics Layer", static_cast<uint32>(bcc.layer_index));
            writer.write(u8"Offset", ConfigValue(bcc.offset));
            writer.write(u8"Size", ConfigValue(bcc.size));
            writer.write(u8"Density", ConfigValue(bcc.density));
            writer.write(u8"Friction", ConfigValue(bcc.friction));
            writer.write(u8"Restitution", ConfigValue(bcc.restitution));

            writer.end_map();
        }
        if(entity.has<NativeScriptComponent>())
        {
            writer.begin_map(u8"Native-Script Component");

            const auto& nsc{ entity.get<NativeScriptComponent>() };
            writer.write(u8"Script Name", ConfigValue(nsc.script_name));
            writer.write(u8"Wants to Update", ConfigValue(nsc.wants_to_update));

            writer.end_map();
        }

        writer.end_map();
    }

    static void deserialize_entity(Scene& scene, const ConfigReader& reader)
    {
        UUID uuid{ reader.get_at(u8"Entity", 0ui64) };
        String name{ reader.get_at(u8"Tag", String{}) };
        Entity entity{ scene.create_entity(name, uuid) };

        Scope<ConfigReader> tc_reader{ reader.get_child(u8"Transform Component") };
        if(tc_reader) {
            auto& tc{ entity.get_mut<TransformComponent>() };
            tc.position = tc_reader->get_at(u8"Position", tc.position);
            tc.rotation = tc_reader->get_at(u8"Rotation", tc.rotation);
            tc.size     = tc_reader->get_at(u8"Size"    , tc.size    );
            tc.locked   = tc_reader->get_at(u8"Locked"  , tc.locked  );
        }

        Scope<ConfigReader> cc_reader{ reader.get_child(u8"Camera Component") };
        if(cc_reader) {
            auto& cc{ entity.emplace<CameraComponent>() };
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

        Scope<ConfigReader> sc_reader{ reader.get_child(u8"Sprite Component") };
        if(sc_reader) {
            auto& sc{ entity.emplace<SpriteComponent>
                (AssetUUID(sc_reader->get_at(u8"Texture", 0ui64)))};
            Scope<ConfigReader> tex_config{ sc_reader->get_child(u8"Settings") };
            if(sc.sprite.has_texture() && tex_config) {
                sc.sprite.tiling_factor = tex_config->get_at(u8"Tiling Factor", 1.0f);
                sc.sprite.cell_pixels = tex_config->get_at(u8"Cell Pixels", glm::vec2(1.0f));
                sc.sprite.cell_coords = tex_config->get_at(u8"Cell Coords", glm::vec2(0.0f));
                sc.sprite.cell_counts = tex_config->get_at(u8"Cell Counts", glm::vec2(1.0f));
            }
            sc.color = sc_reader->get_at(u8"Color", glm::vec4(1.0f));
            sc.blending_mode = static_cast<SpriteComponent::BlendingMode>
                (sc_reader->get_at(u8"Blending Mode", 0ui32));
            sc.rendering_layer = sc_reader->get_at(u8"Rendering Layer", 0);
        }

        Scope<ConfigReader> rbc_reader{ reader.get_child(u8"Rigidbody 2D Component") };
        if(rbc_reader) {
            auto& rbc{ entity.emplace<Rigidbody2DComponent>() };
            rbc.type = static_cast<BodyType>(rbc_reader->get_at(u8"Type", 0ui32));
            rbc.rotation_fixed = rbc_reader->get_at(u8"Rotation Fixed", false);
        }

        Scope<ConfigReader> bcc_reader{ reader.get_child(u8"Box Collider 2D Component") };
        if(bcc_reader) {
            auto& bcc{ entity.emplace<BoxCollider2DComponent>() };
            bcc.type = static_cast<ColliderType>(bcc_reader->get_at(u8"Collider Type", 0u));
            bcc.layer_index = bcc_reader->get_at(u8"Physics Layer", 0ui32);
            bcc.offset      = bcc_reader->get_at(u8"Offset", glm::vec2(0.0f));
            bcc.size        = bcc_reader->get_at(u8"Size", glm::vec2(0.5f));
            bcc.density     = bcc_reader->get_at(u8"Density", 1.0f);
            bcc.friction    = bcc_reader->get_at(u8"Friction", 0.5f);
            bcc.restitution = bcc_reader->get_at(u8"Restitution", 0.0f);
        }

        Scope<ConfigReader> nsc_reader{ reader.get_child(u8"Native-Script Component") };
        if(nsc_reader) {
            auto& nsc{ entity.emplace<NativeScriptComponent>() };
            nsc.script_name     = nsc_reader->get_at(u8"Script Name", String{});
            nsc.wants_to_update = nsc_reader->get_at(u8"Wants to Update", true);
        }
    }
}

namespace rke
{
    bool SceneSerializer::serialize(const Scene& scene, const Path& filepath)
    {
        Scope<ConfigWriter> writer{ ConfigWriter::create() };
        writer->begin_map();

        writer->write(u8"Scene", scene.get_name());
        writer->write(u8"Gravity", scene.get_gravity());

        writer->begin_array(u8"Entities");
        auto view{ scene.registry_->view<IdentityComponent>() };
        for(auto it{ view.rbegin() }; it != view.rend(); ++it)
        {
            Entity entity{ scene.get_entity(static_cast<uint32>(*it)) };
            if(!entity.valid()) continue;
            serialize_entity(*(writer.get()), entity);
        }
        writer->end_array();

        Entity selected{ scene.get_selected_entity() };
        writer->write(u8"Selected Entity", selected.valid() ? selected.get_uuid().value() : 0);

        Entity master_cam{ scene.get_master_camera() };
        writer->write(u8"Master Camera", master_cam.valid() ? master_cam.get_uuid().value() : 0);

        Entity demo_cam{ scene.get_demo_camera() };
        writer->write(u8"Demo Camera", demo_cam.valid() ? demo_cam.get_uuid().value() : 0);

        if(serialize_hook_) serialize_hook_(scene, *(writer.get()));

        writer->end_map();

        file::check_to_create_dir(filepath);
        if(writer->push_to_file(filepath))
            { scene.modified_ = false; return true; }

        CORE_ERROR(u8"SceneSerializer: Failed to serialize scene '{}'!", filepath);
        return false;
    }

    bool SceneSerializer::deserialize(Scene& scene, const Path& filepath)
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
        scene.set_name(reader->get_at(u8"Scene", String{}));
        scene.get_gravity_mut() = reader->get_at(u8"Gravity", Gravity2D::get_default());

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
        entities->for_each([&scene](Scope<ConfigReader> config)
            { deserialize_entity(scene, *(config.get())); });

        if(reader->has_key(u8"Selected Entity")) {
            UUID uuid{ reader->get_at(u8"Selected Entity", 0ui64) };
            scene.set_selected_entity(uuid);
        }
        else scene.set_selected_entity(Entity{});

        if(reader->has_key(u8"Master Camera")) {
            UUID uuid{ reader->get_at(u8"Master Camera", 0ui64) };
            scene.set_master_camera(uuid);
        }
        else scene.set_master_camera(Entity{});

        if(reader->has_key(u8"Demo Camera")) {
            UUID uuid{ reader->get_at(u8"Demo Camera", 0ui64) };
            scene.set_demo_camera(uuid);
        }
        else scene.set_demo_camera(Entity{});

        if(deserialize_hook_) deserialize_hook_(scene, *(reader.get()));

        scene.get_master_camera();
        scene.modified_ = false; // just loaded, nothing changed
        return true;
    }

    void SceneSerializer::set_serialize_hook(SerializeHook hook)
        { serialize_hook_ = std::move(hook); }
    void SceneSerializer::set_deserialize_hook(DeserializeHook hook)
        { deserialize_hook_ = std::move(hook); }
}
