#include "DMD_Reader.h"

#include <iostream>
#include <set>

DMD_Reader::DMD_Reader()
{
    supportedExtensions.insert(".dmd");
}

vsg::ref_ptr<vsg::MatrixTransform> DMD_Reader::read(
    const vsg::Path& model_path,
    vsg::ref_ptr<vsg::Data> texture_data,
    vsg::ref_ptr<const vsg::Options> options
)
{
    auto string_value = read_cast<vsg::stringValue>(model_path, options);
    if (!string_value)
    {
        std::cerr << "Failed to open \"" << model_path << "\" for reading\n";
        return vsg::ref_ptr<vsg::MatrixTransform>();
    }

    std::stringstream stream(string_value->value());

    std::string str;

    bool object_added = false;
    bool numverts_readed = false;
    bool vertices_readed = false;
    bool faces_readed = false;
    bool numtverts_readed = false;
    bool tverts_readed = false;
    bool tfaces_readed = false;

    while (std::getline(stream, str))
    {
        remove_CR_symbols(str);

        if (str == "New object" && !object_added)
        {
            add_mesh();
            object_added = true;
        }
        else if (str == "numverts numfaces" && !numverts_readed)
        {
            read_numverts_and_numfaces(stream);
            init_temp_arrays();
            numverts_readed = true;
        }
        else if (str == "Mesh vertices:" && !vertices_readed)
        {
            read_mesh_vertices(stream);
            vertices_readed = true;
        }
        else if (str == "Mesh faces:" && !faces_readed)
        {
            read_mesh_faces(stream);
            faces_readed = true;
        }
        else if (str == "numtverts numtvfaces" && !numtverts_readed)
        {
            read_numtverts_and_numtvfaces(stream);
            init_mesh_arrays();
            set_default_color();
            numtverts_readed = true;
        }
        else if (str == "Texture vertices:" && !tverts_readed)
        {
            read_texture_vertices(stream);
            tverts_readed = true;
        }
        else if (str == "Texture faces:" && !tfaces_readed)
        {
            read_and_proceed_texture_faces(stream);
            calculate_vertex_normals();
            tfaces_readed = true;
        }
    }

    if (!object_added || !numverts_readed || !vertices_readed || !faces_readed
        || !numtverts_readed || !tverts_readed || !tfaces_readed)
    {
        return vsg::ref_ptr<vsg::MatrixTransform>();
    }

    reset_mesh_arrays();

    combine_meshes_array();

    vsg::DescriptorSetLayoutBindings descriptor_bindings{{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    }};

    auto descriptor_set_layout =
        vsg::DescriptorSetLayout::create(descriptor_bindings);

    vsg::PushConstantRanges push_constant_ranges{{
        VK_SHADER_STAGE_VERTEX_BIT, 0, 128
    }};

    vsg::VertexInputState::Bindings vertex_bindings_description{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3),
                                        VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3),
                                        VK_VERTEX_INPUT_RATE_VERTEX},
        VkVertexInputBindingDescription{2, sizeof(vsg::vec2),
                                        VK_VERTEX_INPUT_RATE_VERTEX}
    };

    vsg::VertexInputState::Attributes vertex_attribute_descriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},
    };

    vsg::GraphicsPipelineStates pipeline_states{
        vsg::VertexInputState::create(vertex_bindings_description,
                                      vertex_attribute_descriptions),
        vsg::InputAssemblyState::create(),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()
    };

    auto pipeline_layout = vsg::PipelineLayout::create(
        vsg::DescriptorSetLayouts{descriptor_set_layout},
        push_constant_ranges
    );



    auto shader_set = vsg::createPhongShaderSet(options);
    if (!shader_set)
    {
        std::cerr << "Failed to create shader set\n";
        return vsg::ref_ptr<vsg::MatrixTransform>();
    }

    auto graphics_pipeline_config = vsg::GraphicsPipelineConfigurator::create(shader_set);

    vsg::DataList vertex_arrays;
    graphics_pipeline_config->assignArray(vertex_arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, m_model_vertices);
    graphics_pipeline_config->assignArray(vertex_arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, m_model_normals);
    graphics_pipeline_config->assignArray(vertex_arrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, m_model_tex_coords);
    graphics_pipeline_config->assignArray(vertex_arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, m_model_colors);

    auto draw_commands = vsg::Commands::create();
    draw_commands->addChild(vsg::BindVertexBuffers::create(graphics_pipeline_config->baseAttributeBinding, vertex_arrays));
    draw_commands->addChild(vsg::BindIndexBuffer::create(m_model_indices));
    draw_commands->addChild(vsg::DrawIndexed::create(m_model_indices->size(), 1, 0, 0, 0));

    if (texture_data)
    {
        graphics_pipeline_config->assignTexture("diffuseMap", texture_data);
    }

    graphics_pipeline_config->init();

    auto state_group = vsg::StateGroup::create();
    graphics_pipeline_config->copyTo(state_group);
    state_group->addChild(draw_commands);

    // auto paged_LOD = vsg::PagedLOD::create();
    // paged_LOD->filename = model_path;
    // paged_LOD->options = options;
    // paged_LOD->children[0].minimumScreenHeightRatio = 0.0;
    // paged_LOD->children[0].node = state_group;

    auto matrix_transform = vsg::MatrixTransform::create();
    // matrix_transform->addChild(paged_LOD);
    matrix_transform->addChild(state_group);

    return matrix_transform;
}

void DMD_Reader::remove_CR_symbols(std::string& str)
{
    while (true)
    {
        auto CR_pos = str.find('\r');
        if (CR_pos == std::string::npos)
        {
            break;
        }
        str.erase(CR_pos);
    }
}

void DMD_Reader::add_mesh()
{
    m_current_mesh = &(m_meshes.emplace_back(Mesh()));
    reset_mesh_arrays();
}

void DMD_Reader::reset_mesh_arrays()
{
    m_temp_vertices.reset();
    m_temp_indices.reset();
    m_vertices_count = 0;
    m_faces_count = 0;
    m_indices_count = 0;
    m_temp_vertices_count = 0;
}

void DMD_Reader::read_numverts_and_numfaces(std::stringstream& stream)
{
    stream >> m_temp_vertices_count >> m_faces_count;
    m_indices_count = m_faces_count * 3;
}

void DMD_Reader::init_temp_arrays()
{
    m_temp_vertices = vsg::vec3Array::create(m_temp_vertices_count);
    m_temp_indices = vsg::ushortArray::create(m_indices_count);
}

void DMD_Reader::read_mesh_vertices(std::stringstream& stream)
{
    for (auto& tempVertex : *m_temp_vertices)
    {
        stream >> tempVertex;
    }
}

void DMD_Reader::read_mesh_faces(std::stringstream& stream)
{
    for (auto& tempIndex : *m_temp_indices)
    {
        stream >> tempIndex;
        --tempIndex;
    }
}

void DMD_Reader::read_numtverts_and_numtvfaces(std::stringstream& stream)
{
    stream >> m_vertices_count >> m_faces_count;
}

void DMD_Reader::init_mesh_arrays()
{
    m_current_mesh->vertices = vsg::vec3Array::create(m_vertices_count);
    m_current_mesh->normals = vsg::vec3Array::create(m_vertices_count);
    m_current_mesh->tex_coords = vsg::vec3Array::create(m_vertices_count);
    m_current_mesh->colors = vsg::vec4Array::create(m_vertices_count);
    m_current_mesh->indices = vsg::ushortArray::create(m_indices_count);
}

void DMD_Reader::read_texture_vertices(std::stringstream& stream)
{
    for (auto& texCoord : *m_current_mesh->tex_coords)
    {
        stream >> texCoord;
    }
}

void DMD_Reader::set_default_color()
{
    for (auto& color : *m_current_mesh->colors)
    {
        color.set(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void DMD_Reader::read_and_proceed_texture_faces(std::stringstream& stream)
{
    std::set<std::size_t> processed_indices;
    for (std::size_t i = 0; i < m_indices_count; ++i)
    {
        auto& index = m_current_mesh->indices->at(i);
        stream >> index;
        --index;

        // Если вершина с заданным индексом еще не была обработана,
        // привязать координаты этой вершины
        // к текстурным координатам
        if (processed_indices.find(index) == processed_indices.end())
        {
            std::size_t temp_index = m_temp_indices->at(i);
            m_current_mesh->vertices->at(index) = m_temp_vertices->at(temp_index);
            processed_indices.insert(index);
        }
    }
}

void DMD_Reader::calculate_vertex_normals()
{
    for (std::size_t i = 0; i < m_faces_count; ++i)
    {
        std::size_t index1 = m_current_mesh->indices->at(i * 3);
        std::size_t index2 = m_current_mesh->indices->at(i * 3 + 1);
        std::size_t index3 = m_current_mesh->indices->at(i * 3 + 2);

        vsg::vec3& vertex1 = m_current_mesh->vertices->at(index1);
        vsg::vec3& vertex2 = m_current_mesh->vertices->at(index2);
        vsg::vec3& vertex3 = m_current_mesh->vertices->at(index3);

        vsg::vec3& vertex_normal1 = m_current_mesh->normals->at(index1);
        vsg::vec3& vertex_normal2 = m_current_mesh->normals->at(index2);
        vsg::vec3& vertex_normal3 = m_current_mesh->normals->at(index3);

        vsg::vec3 face_normal = vsg::cross(vertex2 - vertex1,
                                           vertex3 - vertex1);

        // Добавить к каждой нормали вершины нормаль поверхности,
        // на которой она лежит
        vertex_normal1 += face_normal;
        vertex_normal2 += face_normal;
        vertex_normal3 += face_normal;
    }
}

void DMD_Reader::combine_meshes_array()
{
    std::size_t model_vertices_count = 0;
    std::size_t model_indices_count = 0;

    for (auto& mesh : m_meshes)
    {
        model_vertices_count += mesh.vertices->size();
        model_indices_count += mesh.indices->size();
    }

    m_model_vertices = vsg::vec3Array::create(model_vertices_count);
    m_model_normals = vsg::vec3Array::create(model_vertices_count);
    m_model_tex_coords = vsg::vec3Array::create(model_vertices_count);
    m_model_colors = vsg::vec4Array::create(model_vertices_count);
    m_model_indices = vsg::ushortArray::create(model_indices_count);

    model_vertices_count = 0;
    model_indices_count = 0;

    for (auto& mesh : m_meshes)
    {
        std::size_t mesh_vertices_count = mesh.vertices->size();
        for (std::size_t i = 0; i < mesh_vertices_count; ++i)
        {
            m_model_vertices->at(i + model_vertices_count) = mesh.vertices->at(i);
            m_model_normals->at(i + model_vertices_count) = mesh.normals->at(i);
            m_model_tex_coords->at(i + model_vertices_count) = mesh.tex_coords->at(i);
            m_model_colors->at(i + model_vertices_count) = mesh.colors->at(i);
        }

        std::size_t mesh_indices_count = mesh.indices->size();
        for (std::size_t i = 0; i < mesh_indices_count; ++i)
        {
            m_model_indices->at(i + model_indices_count) = mesh.indices->at(i);
        }

        model_vertices_count += mesh_vertices_count;
        model_indices_count += mesh_indices_count;
    }

    m_meshes.clear();
}
