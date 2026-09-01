module;

#include <memory>
#include <glm/glm.hpp>
#include "rke_macros.h"

export module Mesh;

import Types;
import HeapManager;

export namespace rke
{
    class RKE_API Mesh
    {
    public:
        Mesh(uint32 vc, uint32 ic,
            glm::vec3 centre, glm::vec3 size, glm::vec3 front,
            Scope<glm::vec4[]> pos, Scope<uint32[]> indices,
            Scope<glm::vec3[]> nor = nullptr,
            Scope<glm::vec4[]> col = nullptr,
            Scope<glm::vec2[]> uvs = nullptr);
        // Mesh(const Path& filepath);
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        inline uint32 get_vertex_count() const { return vertex_count_; }
        inline uint32 get_index_count() const { return index_count_; }

        inline glm::vec3 get_centre() const { return centre_; }
        inline glm::vec3 get_size() const { return size_; }
        inline glm::vec3 get_front() const { return front_; }

        inline bool has_normals() const { return normals_.get() != nullptr; }
        inline bool has_colors() const { return colors_.get() != nullptr; }
        inline bool has_uvs() const { return uvs_.get() != nullptr; }

        const glm::vec4* get_position(uint32 index) const;
        const glm::vec3* get_normal(uint32 index) const;
        const glm::vec4* get_color(uint32 index) const;
        const glm::vec2* get_uv(uint32 index) const;
        const uint32* get_index(uint32 index) const;
    private:
        uint32 vertex_count_;
        uint32 index_count_;

        glm::vec3 centre_;
        glm::vec3 size_;
        glm::vec3 front_;

        Scope<glm::vec4[]> positions_;
        Scope<glm::vec3[]> normals_;
        Scope<glm::vec4[]> colors_;
        Scope<glm::vec2[]> uvs_;
        Scope<uint32[]> indices_;
    };
}

