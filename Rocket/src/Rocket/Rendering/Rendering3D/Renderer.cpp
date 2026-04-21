module;
module Renderer;

namespace rke
{
    void Renderer::init()	  {}
    void Renderer::shutdown() {}

    void Renderer::submit (
        const Ref<Shader>& shader, const Ref<VertexArray>& vao,
        const glm::mat4& mod_mat)
    {
        shader->bind();
        vao   ->bind();
        RenderCommand::draw_indexed(vao);
    }
}
