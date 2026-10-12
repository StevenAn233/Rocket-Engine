module;
module Mesh;

import Log;

namespace
{
    using namespace rke;
    static uint64 edge_key(uint32 a, uint32 b)
    {
        const uint64 lo{ a < b ? a : b };
        const uint64 hi{ a < b ? b : a };
        return (hi << 32) | lo;
    }
}

namespace rke
{
    Mesh::Mesh(uint32 vc, uint32 ic,
        Scope<glm::vec4[]> pos, Scope<uint32[]> indices,
        Scope<glm::vec3[]> nor,
        Scope<glm::vec4[]> col,
        Scope<glm::vec2[]> uvs)
        : vertex_count_(vc), index_count_(ic)
        , positions_(std::move(pos))
        , normals_(std::move(nor))
        , colors_(std::move(col))
        , uvs_(std::move(uvs))
        , indices_(std::move(indices))
    {
        CORE_ASSERT(positions_, u8"Mesh: Position data null!");
        CORE_ASSERT(indices_,   u8"Mesh: Index data null!"   );
        CORE_ASSERT(vc > 0,     u8"Mesh: Zero vertices!"     );

    // Only xyz take part: the w component is homogeneous, not a spatial axis.
        bounds_min_ = glm::vec3(*positions_.get()); // the first element
        bounds_max_ = bounds_min_;
        for(uint32 i{ 1 }; i < vertex_count_; i++)
        {
            const glm::vec3 p{ glm::vec3(*(positions_.get() + i)) };
            bounds_min_ = glm::min(bounds_min_, p);
            bounds_max_ = glm::max(bounds_max_, p);
        }

    // Generate Edges
        std::unordered_map<uint64, Size> lookup{}; // for calc face valence
        for(uint32 i{}; i + 2 < index_count_; i += 3)
        {
            const uint32* index_a{ get_index(i + 0) };
            const uint32* index_b{ get_index(i + 1) };
            const uint32* index_c{ get_index(i + 2) };
            if(!index_a || !index_b || !index_c) break;
            const uint32 tri[3]{ *index_a, *index_b, *index_c };

            for(uint32 i{}; i < 3; i++)
            {
                const uint32 a{ tri[i] }, b{ tri[(i + 1) % 3] };
                const uint64 key{ edge_key(a, b) };
                auto it{ lookup.find(key) };
                if(it != lookup.end())
                {
                    edges_[it->second].face_valence++;
                    continue;
                }
                lookup.emplace(key, edges_.size());
                edges_.push_back(Edge{ .a = a, .b = b, .face_valence = 1 });
            }
        }
    }

    const glm::vec4* Mesh::get_position(uint32 index) const
    {
        CORE_ASSERT(index < vertex_count_, u8"Mesh: Out of bound!");
        return positions_.get() + index;
    }

    const glm::vec3* Mesh::get_normal(uint32 index) const
    {
        CORE_ASSERT(index < vertex_count_, u8"Mesh: Out of bound!");
        if(!normals_) return nullptr;
        return normals_.get() + index;
    }

    const glm::vec4* Mesh::get_color(uint32 index) const
    {
        CORE_ASSERT(index < vertex_count_, u8"Mesh: Out of bound!");
        if(!colors_) return nullptr;
        return colors_.get() + index;
    }

    const glm::vec2* Mesh::get_uv(uint32 index) const
    {
        CORE_ASSERT(index < vertex_count_, u8"Mesh: Out of bound!");
        if(!uvs_) return nullptr;
        return uvs_.get() + index;
    }

    const uint32* Mesh::get_index(uint32 index) const
    {
        CORE_ASSERT(index < index_count_, u8"Mesh: Out of bound!");
        return indices_.get() + index;
    }
}

namespace rke::mesh
{
    const Mesh& quad()
    {
        static const Mesh s_quad(4, 6,
            Scope<glm::vec4[]>(new glm::vec4[4]
            {
                glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
                glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f),
                glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
                glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f)
            }),
            Scope<uint32[]>(new uint32[6]{ 0, 1, 2, 2, 3, 0 }),
            nullptr, nullptr,
            Scope<glm::vec2[]>(new glm::vec2[4]
            {
                glm::vec2(1.0f, 1.0f),
                glm::vec2(0.0f, 1.0f),
                glm::vec2(0.0f, 0.0f),
                glm::vec2(1.0f, 0.0f)
            })
        );
        return s_quad;
    }
}
