#include "DMD_Reader.h"

#include "Mesh.h"

#include <fstream>
#include <iostream>
#include <set>

#include <stb_image.h>

vsg::ref_ptr<vsg::DescriptorSetLayout>  DMD_Reader::descriptorSetLayout;
vsg::ref_ptr<vsg::PipelineLayout>       DMD_Reader::pipelineLayout;
vsg::ref_ptr<vsg::BindGraphicsPipeline> DMD_Reader::bindGraphicsPipeline;

vsg::ref_ptr<vsg::Object> DMD_Reader::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
    vsg::ref_ptr<vsg::SharedObjects> sharedObjects = options->sharedObjects;

    const size_t dot_dmd_pos = filename.find(".dmd");
    if (dot_dmd_pos == filename.npos)
    {
        return vsg::StateGroup::create();
    }

    vsg::Path model_path;
    vsg::Path texture_path;

    std::stringstream stream(filename);
    stream >> model_path;
    stream >> texture_path;

    vsg::ref_ptr<ModelData> model_data;
    const vsg::Path model_file = vsg::findFile(model_path, options);
    if (!model_file || (vsg::fileExtension(model_file) != ".dmd"))
    {
        return vsg::StateGroup::create();
    }

    model_data = load_model(model_file);

    if (!model_data)
    {
        return vsg::StateGroup::create();
    }

    vsg::ref_ptr<vsg::Data> texture_data;
    stbi_uc* pixels = NULL;
    const vsg::Path textureFile = vsg::findFile(texture_path, options);
    if (vsg::fileExtension(textureFile) == ".bmp")
    {
        stbi_set_flip_vertically_on_load(1);
    }
    else
    {
        stbi_set_flip_vertically_on_load(0);
    }

    int width, height, channels;
    pixels = stbi_load(textureFile.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    texture_data = vsg::ubvec4Array2D::create(width, height, reinterpret_cast<vsg::ubvec4*>(pixels), vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM});

    auto pipeline = vsg::GraphicsPipelineConfigurator::create(options->shaderSets.at("phong"));

    vsg::DataList vertexArrays;
    pipeline->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, model_data->vertices);
    pipeline->assignArray(vertexArrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, model_data->normals);
    pipeline->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, model_data->tex_coords);
    pipeline->assignArray(vertexArrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, model_data->colors);

    sharedObjects->share(vertexArrays);
    sharedObjects->share(model_data->indices);

    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(pipeline->baseAttributeBinding, vertexArrays));
    drawCommands->addChild(vsg::BindIndexBuffer::create(model_data->indices));
    drawCommands->addChild(vsg::DrawIndexed::create(model_data->indices->size(), 1, 0, 0, 0));

    sharedObjects->share(drawCommands->children);
    sharedObjects->share(drawCommands);

    if (texture_data)
    {
        auto sampler = vsg::Sampler::create();
        sampler->maxLod = 10.0f;
        sharedObjects->share(sampler);
        pipeline->assignTexture("diffuseMap", texture_data, sampler);
    }

    sharedObjects->share(pipeline, [](auto gpc) { gpc->init(); });

    auto stateGroup = vsg::StateGroup::create();
    pipeline->copyTo(stateGroup);
    stateGroup->addChild(drawCommands);
    sharedObjects->share(stateGroup);

    return stateGroup;
}

struct vertex_t
{
    vsg::vec3 pos;
    vsg::vec3 normal;
    vsg::vec2 tex_coord;
};

static bool equal(float a, float b)
{
    return std::fabs(a - b) < 0.000001;
}

vsg::ref_ptr<ModelData> DMD_Reader::load_model(const vsg::Path& path) const
{
    std::ifstream inf(path);
    if (!inf)
    {
        return {};
    }

    std::string buf;
    while (buf != "TriMesh()")
    {
        inf >> buf;
    }

    inf >> buf >> buf;

    std::uint32_t temp_vertex_count, temp_face_count;
    inf >> temp_vertex_count >> temp_face_count;

    inf >> buf >> buf;

    std::vector<vsg::vec3> temp_vertices(temp_vertex_count);
    for (vsg::vec3& vertex : temp_vertices)
    {
        inf >> buf;
        if (buf.find('#') != std::string::npos)
        {
            return {};
        }
        vertex.x = std::stof(buf);

        inf >> buf;
        if (buf.find('#') != std::string::npos)
        {
            return {};
        }
        vertex.y = std::stof(buf);

        inf >> buf;
        if (buf.find('#') != std::string::npos)
        {
            return {};
        }
        vertex.z = std::stof(buf);
    }

    inf >> buf >> buf >> buf >> buf;

    std::vector<std::uint32_t> temp_vertex_indices(temp_face_count * 3);
    for (std::uint32_t& index : temp_vertex_indices)
    {
        inf >> index;
        --index;
    }

    while (buf != "Texture:")
    {
        inf >> buf;
    }

    inf >> buf >> buf;

    std::uint32_t temp_tex_coord_count;
    inf >> temp_tex_coord_count >> temp_face_count;

    inf >> buf >> buf;

    std::vector<vsg::vec2> temp_tex_coords(temp_tex_coord_count);
    for (vsg::vec2& tex_coord : temp_tex_coords)
    {
        inf >> tex_coord.x >> tex_coord.y >> buf;
    }

    inf >> buf >> buf >> buf >> buf >> buf;

    std::vector<std::uint32_t> temp_tex_coord_indices(temp_face_count * 3);
    for (std::uint32_t& index : temp_tex_coord_indices)
    {
        inf >> index;
        --index;
    }

    std::vector<vertex_t> vertices(temp_face_count * 3);
    for (std::uint32_t i = 0; i < temp_face_count * 3; ++i)
    {
        vertices[i].pos = temp_vertices[temp_vertex_indices[i]];
        vertices[i].tex_coord = temp_tex_coords[temp_tex_coord_indices[i]];
    }

    temp_tex_coord_indices.clear();
    temp_tex_coords.clear();
    temp_vertex_indices.clear();
    temp_vertices.clear();

    std::vector<std::uint32_t> indices(temp_face_count * 3);
    for (std::uint32_t i = 0; i < temp_face_count * 3; ++i)
    {
        indices[i] = i;
    }

    for (std::uint32_t i = 0; i < indices.size(); i += 3)
    {
        std::uint32_t index_1 = indices[i];
        std::uint32_t index_2 = indices[i + 1];
        std::uint32_t index_3 = indices[i + 2];

        const vsg::vec3& pos_1 = vertices[index_1].pos;
        const vsg::vec3& pos_2 = vertices[index_2].pos;
        const vsg::vec3& pos_3 = vertices[index_3].pos;

        vsg::vec3& normal_1 = vertices[index_1].normal;
        vsg::vec3& normal_2 = vertices[index_2].normal;
        vsg::vec3& normal_3 = vertices[index_3].normal;

        vsg::vec3 face_normal = vsg::cross(pos_2 - pos_1, pos_3 - pos_1);

        normal_1 += face_normal;
        normal_2 += face_normal;
        normal_3 += face_normal;
    }

    for (vertex_t& vertex : vertices)
    {
        vertex.normal = vsg::normalize(vertex.normal);
    }

    for (std::uint32_t i = 0; i < vertices.size(); ++i)
    {
        const vertex_t& vertex_1 = vertices[i];
        for (std::uint32_t j = i + 1; j < vertices.size(); ++j)
        {
            const vertex_t& vertex_2 = vertices[j];
            if (equal(vertex_1.pos.x, vertex_2.pos.x)
                && equal(vertex_1.pos.y, vertex_2.pos.y)
                && equal(vertex_1.pos.z, vertex_2.pos.z)
                && equal(vertex_1.normal.x, vertex_2.normal.x)
                && equal(vertex_1.normal.y, vertex_2.normal.y)
                && equal(vertex_1.normal.z, vertex_2.normal.z)
                && equal(vertex_1.tex_coord.x, vertex_2.tex_coord.x)
                && equal(vertex_1.tex_coord.y, vertex_2.tex_coord.y))
            {
                for (std::uint32_t& index : indices)
                {
                    if (index == j)
                    {
                        index = i;
                    }
                    else if (index > j)
                    {
                        --index;
                    }
                }

                vertices.erase(vertices.begin() + j);

                --j;
            }
        }
    }

    for (std::uint32_t i = 0; i < indices.size(); i += 3)
    {
        std::uint32_t index_1 = indices[i];
        std::uint32_t index_2 = indices[i + 1];
        std::uint32_t index_3 = indices[i + 2];

        if (index_1 == index_2 || index_1 == index_3 || index_2 == index_3)
        {
            for (std::uint32_t j = 0; j < 3; ++j)
            {
                indices.erase(indices.begin() + i);
            }
            i -= 3;
        }
        else
        {
            for (std::uint32_t j = i + 3; j < indices.size(); j += 3)
            {
                std::uint32_t index_4 = indices[j];
                std::uint32_t index_5 = indices[j + 1];
                std::uint32_t index_6 = indices[j + 2];

                if (index_1 == index_4 && index_2 == index_5 && index_3 == index_6)
                {
                    for (std::uint32_t k = 0; k < 3; ++k)
                    {
                        indices.erase(indices.begin() + j);
                    }
                }
            }
        }
    }

    auto model_data = ModelData::create();
    model_data->vertices = vsg::vec3Array::create(vertices.size());
    model_data->normals = vsg::vec3Array::create(vertices.size());
    model_data->tex_coords = vsg::vec2Array::create(vertices.size());
    model_data->colors = vsg::vec4Array::create(vertices.size());
    model_data->indices = vsg::ushortArray::create(indices.size());

    for (std::uint32_t i = 0; i < vertices.size(); ++i)
    {
        model_data->vertices->at(i).set(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        model_data->normals->at(i).set(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z);
        model_data->tex_coords->at(i).set(vertices[i].tex_coord.x, vertices[i].tex_coord.y);
        model_data->colors->at(i).set(1.0f, 1.0f, 1.0f, 1.0f);
    }

    for (std::uint32_t i = 0; i < indices.size(); ++i)
    {
        model_data->indices->at(i) = indices.at(i);
    }

    return model_data;
}

void DMD_Reader::remove_carriage_return_symbols(std::string& str) const
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

//------------------------------------------------------------------------------
// layout(location = 0) in vec3 vsg_Vertex;
// layout(location = 1) in vec3 vsg_Normal;
// layout(location = 2) in vec2 vsg_TexCoord0;
// layout(location = 3) in vec4 vsg_Color;
//------------------------------------------------------------------------------
// layout(set = MATERIAL_DESCRIPTOR_SET, binding = 10) uniform MaterialData
// {
// vec4 ambientColor;
// vec4 diffuseColor;
// vec4 specularColor;
// vec4 emissiveColor;
// float shininess;
// float alphaMask;
// float alphaMaskCutoff;
// } material;
//------------------------------------------------------------------------------
// layout(set = VIEW_DESCRIPTOR_SET, binding = 0) uniform LightData
// {
// vec4 values[2048];
// } lightData;
//------------------------------------------------------------------------------
// layout(set = VIEW_DESCRIPTOR_SET, binding = 2) uniform sampler2DArrayShadow shadowMaps;
//------------------------------------------------------------------------------

void DMD_Reader::init()
{
    vsg::Paths searchPaths({"../"});
    vsg::ref_ptr<vsg::ShaderStage> vertexShader = vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", vsg::findFile("shaders/vert_PushConstants.spv", searchPaths));
    vsg::ref_ptr<vsg::ShaderStage> fragmentShader = vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", vsg::findFile("shaders/frag_PushConstants.spv", searchPaths));

    //!!!
    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} // { binding, descriptorType, descriptorCount, stageFlags, pImmutableSamplers}
    };
    //!!!

    descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

    vsg::PushConstantRanges pushConstantRanges{
        {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection, view, and model matrices, actual push constant calls automatically provided by the VSG's RecordTraversal
    };

    vsg::VertexInputState::Bindings vertexBindingsDescriptions{
        VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // vertex data
        VkVertexInputBindingDescription{1, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX}, // normal data
        VkVertexInputBindingDescription{2, sizeof(vsg::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, // tex coord data
        VkVertexInputBindingDescription{3, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX}  // color data
    };

    vsg::VertexInputState::Attributes vertexAttributeDescriptions{
        VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},   // vertex data
        VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0},   // normal data
        VkVertexInputAttributeDescription{2, 2, VK_FORMAT_R32G32_SFLOAT, 0},      // tex coord data
        VkVertexInputAttributeDescription{3, 3, VK_FORMAT_R32G32B32A32_SFLOAT, 0} // color data
    };

    vsg::GraphicsPipelineStates pipelineStates{
       vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
       vsg::InputAssemblyState::create(),
       vsg::RasterizationState::create(),
       vsg::MultisampleState::create(),
       vsg::ColorBlendState::create(),
       vsg::DepthStencilState::create()
    };

    pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout}, pushConstantRanges);
    auto graphicsPipeline = vsg::GraphicsPipeline::create(pipelineLayout, vsg::ShaderStages{vertexShader, fragmentShader}, pipelineStates);
    bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);
}
