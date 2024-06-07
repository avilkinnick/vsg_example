#include "DMD_Reader.h"

#include "Mesh.h"

#include <fstream>
#include <iostream>
#include <set>

#include <stb_image.h>

std::map<vsg::Path, vsg::ref_ptr<vsg::StateGroup>> DMD_Reader::state_groups;

vsg::ref_ptr<vsg::Object> DMD_Reader::read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
    if (state_groups.find(filename) != state_groups.end())
    {
        return state_groups[filename];
    }

    const size_t dot_dmd_pos = filename.find(".dmd");
    if (dot_dmd_pos == filename.npos)
    {
        state_groups.insert({filename, vsg::StateGroup::create()});
        return state_groups[filename];
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
        state_groups.insert({filename, vsg::StateGroup::create()});
        return state_groups[filename];
    }

    model_data = load_model(model_file);

    if (!model_data)
    {
        state_groups.insert({filename, vsg::StateGroup::create()});
        return state_groups[filename];
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

    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(pipeline->baseAttributeBinding, vertexArrays));
    drawCommands->addChild(vsg::BindIndexBuffer::create(model_data->indices));
    drawCommands->addChild(vsg::DrawIndexed::create(model_data->indices->size(), 1, 0, 0, 0));

    if (texture_data)
    {
        static auto sampler = vsg::Sampler::create();
        pipeline->assignTexture("diffuseMap", texture_data, sampler);
    }

    pipeline->init();

    auto state_group = vsg::StateGroup::create();
    pipeline->copyTo(state_group);
    state_group->addChild(drawCommands);
    state_groups.insert({filename, state_group});

    return state_group;
}

vsg::ref_ptr<ModelData> DMD_Reader::load_model(const vsg::Path& model_file) const
{
    bool object_added = false;
    bool numverts_readed = false;
    bool vertices_readed = false;
    bool faces_readed = false;
    bool numtverts_readed = false;
    bool tverts_readed = false;
    bool tfaces_readed = false;

    std::vector<Mesh> meshes;
    Mesh* current_mesh = nullptr;

    vsg::ref_ptr<vsg::vec3Array>   temp_vertices;
    vsg::ref_ptr<vsg::ushortArray> temp_indices;

    std::size_t vertices_count = 0;
    std::size_t faces_count = 0;
    std::size_t indices_count = 0;
    std::size_t temp_vertices_count = 0;

    std::ifstream file(model_file.string());
    std::string line;

    while (std::getline(file, line))
    {
        remove_carriage_return_symbols(line);

        if (line == "New object" && !object_added)
        {
            current_mesh = &(meshes.emplace_back(Mesh()));

            temp_vertices.reset();
            temp_indices.reset();
            vertices_count = 0;
            faces_count = 0;
            indices_count = 0;
            temp_vertices_count = 0;

            object_added = true;
        }
        else if (line == "numverts numfaces" && !numverts_readed)
        {
            file >> temp_vertices_count >> faces_count;
            indices_count = faces_count * 3;

            temp_vertices = vsg::vec3Array::create(temp_vertices_count);
            temp_indices = vsg::ushortArray::create(indices_count);

            numverts_readed = true;
        }
        else if (line == "Mesh vertices:" && !vertices_readed)
        {
            for (auto& tempVertex : *temp_vertices)
            {
                file >> tempVertex;
            }

            vertices_readed = true;
        }
        else if (line == "Mesh faces:" && !faces_readed)
        {
            for (auto& tempIndex : *temp_indices)
            {
                file >> tempIndex;
                --tempIndex;
            }

            faces_readed = true;
        }
        else if ((line == "numtverts numtvfaces" || line == "numtverts numtfaces") && !numtverts_readed)
        {
            file >> vertices_count >> faces_count;

            current_mesh->vertices = vsg::vec3Array::create(vertices_count);
            current_mesh->normals = vsg::vec3Array::create(vertices_count);
            current_mesh->tex_coords = vsg::vec2Array::create(vertices_count);
            current_mesh->colors = vsg::vec4Array::create(vertices_count);
            current_mesh->indices = vsg::ushortArray::create(indices_count);

            for (auto& color : *current_mesh->colors)
            {
                color.set(1.0f, 1.0f, 1.0f, 1.0f);
            }

            numtverts_readed = true;
        }
        else if (line == "Texture vertices:" && !tverts_readed)
        {
            for (auto& texCoord : *current_mesh->tex_coords)
            {
                file >> texCoord;
                float trash;
                file >> trash;
            }

            tverts_readed = true;
        }
        else if (line == "Texture faces:" && !tfaces_readed)
        {
            std::set<std::size_t> processed_indices;
            for (std::size_t i = 0; i < indices_count; ++i)
            {
                auto& index = current_mesh->indices->at(i);
                file >> index;
                --index;

                // Если вершина с заданным индексом еще не была обработана,
                // привязать координаты этой вершины
                // к текстурным координатам
                if (processed_indices.find(index) == processed_indices.end())
                {
                    std::size_t temp_index = temp_indices->at(i);
                    current_mesh->vertices->at(index) = temp_vertices->at(temp_index);
                    processed_indices.insert(index);
                }
            }

            for (std::size_t i = 0; i < faces_count; ++i)
            {
                std::size_t index1 = current_mesh->indices->at(i * 3);
                std::size_t index2 = current_mesh->indices->at(i * 3 + 1);
                std::size_t index3 = current_mesh->indices->at(i * 3 + 2);

                vsg::vec3& vertex1 = current_mesh->vertices->at(index1);
                vsg::vec3& vertex2 = current_mesh->vertices->at(index2);
                vsg::vec3& vertex3 = current_mesh->vertices->at(index3);

                vsg::vec3& vertex_normal1 = current_mesh->normals->at(index1);
                vsg::vec3& vertex_normal2 = current_mesh->normals->at(index2);
                vsg::vec3& vertex_normal3 = current_mesh->normals->at(index3);

                vsg::vec3 face_normal = vsg::cross(vertex2 - vertex1,
                                                   vertex3 - vertex1);

                // Добавить к каждой нормали вершины нормаль поверхности,
                // на которой она лежит
                vertex_normal1 += face_normal;
                vertex_normal2 += face_normal;
                vertex_normal3 += face_normal;
            }

            tfaces_readed = true;
        }
    }

    if (!object_added || !numverts_readed || !vertices_readed || !faces_readed
        || !numtverts_readed || !tverts_readed || !tfaces_readed)
    {
        return {};
    }

    temp_vertices.reset();
    temp_indices.reset();

    std::size_t model_vertices_count = 0;
    std::size_t model_indices_count = 0;

    for (auto& mesh : meshes)
    {
        model_vertices_count += mesh.vertices->size();
        model_indices_count += mesh.indices->size();
    }

    auto model_data = ModelData::create();
    model_data->vertices = vsg::vec3Array::create(model_vertices_count);
    model_data->normals = vsg::vec3Array::create(model_vertices_count);
    model_data->tex_coords = vsg::vec2Array::create(model_vertices_count);
    model_data->colors = vsg::vec4Array::create(model_vertices_count);
    model_data->indices = vsg::ushortArray::create(model_indices_count);

    model_vertices_count = 0;
    model_indices_count = 0;

    for (auto& mesh : meshes)
    {
        std::size_t mesh_vertices_count = mesh.vertices->size();
        for (std::size_t i = 0; i < mesh_vertices_count; ++i)
        {
            model_data->vertices->at(i + model_vertices_count) = mesh.vertices->at(i);
            model_data->normals->at(i + model_vertices_count) = mesh.normals->at(i);
            model_data->tex_coords->at(i + model_vertices_count) = mesh.tex_coords->at(i);
            model_data->colors->at(i + model_vertices_count) = mesh.colors->at(i);
        }

        std::size_t mesh_indices_count = mesh.indices->size();
        for (std::size_t i = 0; i < mesh_indices_count; ++i)
        {
            model_data->indices->at(i + model_indices_count) = mesh.indices->at(i);
        }

        model_vertices_count += mesh_vertices_count;
        model_indices_count += mesh_indices_count;
    }

    meshes.clear();

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
