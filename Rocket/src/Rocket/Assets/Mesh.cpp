module;
module Mesh;

import Log;

namespace rke
{
    Mesh::Mesh(uint32 vc, uint32 ic, glm::vec3 front,
        Scope<glm::vec4[]> pos, Scope<uint32[]> indices,
        Scope<glm::vec3[]> nor,
        Scope<glm::vec4[]> col,
        Scope<glm::vec2[]> uvs)
        : vertex_count_(vc), index_count_(ic)
        , front_(front)
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
