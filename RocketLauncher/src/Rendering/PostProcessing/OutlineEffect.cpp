module;
module OutlineEffect;

namespace rke
{
    static PostProcessEffect::TypeIdentity s_rke_ppetid
    {
        .type{ u8"OutlineEffect" },
        .shaders {
            { u8"Outline", Shader(file::assets_dir() / u8"shaders" / u8"outline_effect.rkshdr") },
            { u8"Copy", Shader(file::assets_dir() / u8"shaders" / u8"copy.rkshdr") }
        }
    };

// public
    OutlineEffect::OutlineEffect(String name, Window* context, std::function<bool()> extra,
        TargetGetter target_getter, CameraGetter camera_getter)
        : PostProcessEffect(&s_rke_ppetid, std::move(name), std::move(extra))
        , context_(context), target_getter_(std::move(target_getter))
        , camera_getter_(std::move(camera_getter))
    {
        CORE_ASSERT(context_, u8"OutlineEffect: Context window null!");
        ubo_ = UniformBuffer::create(sizeof(Uniforms));
        set_color(glm::vec4(1.0f));

        if(!target_getter_) target_getter_ = []() { return std::vector<Target>{}; };
        if(!camera_getter_) camera_getter_ = []() { return glm::mat4(1.0f); };

        outline_fbo_ = FrameBuffer::create({ .attachment_spec
            {{ GTexture::Format::RGBA16F, glm::vec4(0.0f) }}});
        outline_fbo_->set_samples(samples_);
    }

    uint8 OutlineEffect::get_samples_shift() const
    {
        uint8 shift{};
        uint32 samples{ samples_ }; // copy
        while(samples = samples >> 1) ++shift;
        return shift;
    }

    void OutlineEffect::set_samples(uint8 shift)
    {
        constexpr uint8 max_shift{ 5 }; // to modify
        if(shift > max_shift) {
            CORE_WARN(u8"OutlineEffect: Samples too much!");
            return;
        }
        samples_ = (1u << shift);
        outline_fbo_->set_samples(samples_);
    }

    void OutlineEffect::set_color(glm::vec4 color)
    {
        uniforms_.outline_color = math::srgb_to_linear(color);
        ubo_->set_data(&uniforms_, sizeof(Uniforms));
    }

    void OutlineEffect::set_thickness(float thickness) { thickness_ = thickness; }

// private
    bool OutlineEffect::apply(const GTexture2D* source, FrameBuffer* destination)
    {
        if(!source || !destination) return false;

        auto* outline_shader{ get_shader(u8"Outline") };
        auto* copy_shader{ get_shader(u8"Copy") };
        auto* outline_gshader{ outline_shader ? outline_shader->get_gshader() : nullptr };
        auto* copy_gshader{ copy_shader ? copy_shader->get_gshader() : nullptr };
        if(!outline_gshader || !copy_gshader) {
            CORE_ERROR(u8"OutlineEffect: Shaders unavailable!");
            return false; // keep the source texture
        }

        const FrameBuffer::Specification& spec{ destination->get_specification() };
        const glm::vec2 viewport {
            static_cast<float>(spec.width ),
            static_cast<float>(spec.height)
        };

        const auto targets{ target_getter_() };
        if(targets.empty()) return false;

        uint32 index_count{ build_outline(targets, camera_getter_(), viewport) };
        if(index_count == 0 || !vao_) return false;

        outline_fbo_->clear();
        outline_fbo_->upload([this, index_count, outline_gshader]()
        {
            vao_->bind();
            ubo_->bind(BindingPoint::UBO_PostProcess);
            outline_gshader->bind();
            app().render_command().draw_indexed(static_cast<int>(index_count));
            outline_gshader->unbind();
            vao_->unbind();
        });

        destination->clear();
        destination->upload([this, source, copy_gshader]()
        {
            source->bind(BindingPoint::Sampler2D_0);
            outline_fbo_->get_gtexture_attached()->bind(BindingPoint::Sampler2D_1);
            copy_gshader->bind();
            app().render_command().draw_quad();
            copy_gshader->unbind();
        });

        return true;
    }

    void OutlineEffect::serialize_to(ConfigWriter& writer) const
    {
        PostProcessEffect::serialize_to(writer);
        writer.write(u8"Color", math::linear_to_srgb(uniforms_.outline_color));
        writer.write(u8"Thickness", thickness_);
        writer.write(u8"Samples"  , samples_  );
        writer.write(u8"Wireframe", wireframe_);
    }

    void OutlineEffect::deserialize_from(const ConfigReader& reader)
    {
        PostProcessEffect::deserialize_from(reader);
        set_color(reader.get_at(u8"Color", glm::vec4(1.0f)));
        set_thickness(reader.get_at(u8"Thickness", 1.0f));
        samples_ = reader.get_at(u8"Samples", 1);
        outline_fbo_->set_samples(samples_);
        wireframe_ = reader.get_at(u8"Wireframe", false);
    }

    void OutlineEffect::on_imgui_render()
    {
        PostProcessEffect::on_imgui_render();

        layout::two_columns_table<u8"Color">([this]()
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            constexpr ImGuiColorEditFlags flags
            {	ImGuiColorEditFlags_Float
              | ImGuiColorEditFlags_InputRGB
              | ImGuiColorEditFlags_AlphaBar
            };
            glm::vec4 color{ math::linear_to_srgb(uniforms_.outline_color) };
            if(ImGui::ColorEdit4("##color", glm::value_ptr(color), flags)) set_color(color);
        });
        layout::drag_float_control<u8"Thickness">
            (thickness_, 0.1f, 1.0f, glm::vec2(0.0f, 20.0f));
        layout::two_columns_table<u8"Samples">([this]()
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            constexpr const char* opts[]
                { "1x", "2x", "4x", "8x", "16x" };
            int shift{ static_cast<int>(get_samples_shift()) };
            if(ImGui::Combo("##aa", &shift, opts, static_cast<int>(std::size(opts))))
                { set_samples(static_cast<uint8>(shift)); }
        });
        ImGui::Checkbox("Wireframe", &wireframe_);
    }

    void OutlineEffect::on_viewport_resized(uint32 w, uint32 h)
        { outline_fbo_->resize(w, h); }

    void OutlineEffect::ensure_buffers(Size edge_capacity)
    {
        if(vao_ && edge_capacity <= edge_capacity_) return;
    // 4 vertices and 6 indices(quad) per edge.
        edge_capacity_ = edge_capacity;
        vbo_ = VertexBuffer::create(static_cast<uint32>(edge_capacity_ * 4 * sizeof(glm::vec4)));
        ibo_ = IndexBuffer ::create(static_cast<uint32>(edge_capacity_ * 6));
        vao_ = VertexArray ::create();
        vao_->add_vbo(vbo_, GBufferLayout{{ u8"a_pos", GShaderDataType::Float4 }});
        vao_->set_ibo(ibo_);
    }

    uint32 OutlineEffect::build_outline(const std::vector<Target>& targets,
        const glm::mat4& view_proj, glm::vec2 viewport_size)
    {
        if(viewport_size.x <= 0.0f || viewport_size.y <= 0.0f) return 0;
        if(thickness_ <= 0.0f) return 0; // zero width: nothing visible

        Size capacity{};
        for(const Target& target : targets)
        {
            const Mesh* mesh{ target.first };
            if(mesh) capacity += mesh->get_edges().size();
        }
        if(capacity == 0) return 0;
        ensure_buffers(capacity);

        auto* vertices{ reinterpret_cast<glm::vec4*>(vbo_->map(GBuffer::Access::Write)) };
        auto* indices { reinterpret_cast<uint32*>(ibo_->map(GBuffer::Access::Write)) };
        if(!vertices || !indices)
        {
            CORE_ERROR(u8"OutlineEffect: Failed to map the geometry buffers!");
            if(vertices) vbo_->unmap();
            if(indices ) ibo_->unmap();
            return 0;
        }

        const glm::vec2 pixels_per_ndc{ viewport_size * 0.5f };
        auto corner_clip = [](glm::vec2 ndc, glm::vec2 offset, const glm::vec4& clip)
            { return glm::vec4((ndc + offset) * clip.w, clip.z, clip.w); };

        uint32 vertex_count{}, index_count{};
        for(const Target& target : targets)
        {
            const Mesh* mesh{ target.first };
            if(!mesh) continue;

        // CPU side: every endpoint goes through model -> world -> clip here;
        // and the edges are turned into rectangles of constant screen thickness.
            const glm::mat4 world{ target.second };
            const auto& edges{ mesh->get_edges() };
            for(const Mesh::Edge& edge : edges)
            {
                if(!wireframe_ && (edge.face_valence != 1)) continue;

                const glm::vec4* position_a{ mesh->get_position(edge.a) };
                const glm::vec4* position_b{ mesh->get_position(edge.b) };
                if(!position_a || !position_b) continue;

                const glm::vec4 clip_a{ view_proj * (world * (*position_a)) };
                const glm::vec4 clip_b{ view_proj * (world * (*position_b)) };
                if(clip_a.w == 0.0f || clip_b.w == 0.0f) continue; // no projection

                const glm::vec2 ndc_a{ glm::vec2(clip_a) / clip_a.w };
                const glm::vec2 ndc_b{ glm::vec2(clip_b) / clip_b.w };

                const glm::vec2 pixels{ (ndc_b - ndc_a) * pixels_per_ndc };
                if(glm::dot(pixels, pixels) < 0.25f) continue;

                const glm::vec2 dir{ glm::normalize(pixels) }; // a -> b
                const glm::vec2 cross_dir{ glm::vec2(-dir.y, dir.x) };
                const glm::vec2 half{ (0.5f * thickness_) / pixels_per_ndc };

                const glm::vec2 w_stretch{ (edge.face_valence == 1) ? (dir * half) : glm::vec2(0.0f) };
                const glm::vec2 h_stretch{ cross_dir * half };

                vertices[vertex_count + 0] = corner_clip(ndc_a, (-w_stretch - h_stretch), clip_a);
                vertices[vertex_count + 1] = corner_clip(ndc_a, (-w_stretch + h_stretch), clip_a);
                vertices[vertex_count + 2] = corner_clip(ndc_b, ( w_stretch + h_stretch), clip_b);
                vertices[vertex_count + 3] = corner_clip(ndc_b, ( w_stretch - h_stretch), clip_b);
                indices[index_count + 0] = vertex_count + 0;
                indices[index_count + 1] = vertex_count + 1;
                indices[index_count + 2] = vertex_count + 2;
                indices[index_count + 3] = vertex_count + 0;
                indices[index_count + 4] = vertex_count + 2;
                indices[index_count + 5] = vertex_count + 3;
                vertex_count += 4;
                index_count  += 6;
            }
        }

        vbo_->unmap();
        ibo_->unmap();
        return index_count;
    }
}
