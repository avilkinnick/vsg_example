#include "DMD_Reader.h"

#include "Mesh.h"

#include <iostream>
#include <set>

vsg::ref_ptr<vsg::Object> DMD_Reader::read(
    const vsg::Path& filename,
    vsg::ref_ptr<const vsg::Options> options
) const
{
    vsg::Path found_filename = vsg::findFile(filename, options);

    if (vsg::fileExtension(filename) != ".dmd"
        || !found_filename)
    {
        return {};
    }

    std::ifstream stream(filename.string());
    std::string str;

    bool object_added = false;
    bool numverts_readed = false;
    bool vertices_readed = false;
    bool faces_readed = false;
    bool numtverts_readed = false;
    bool tverts_readed = false;
    bool tfaces_readed = false;

    vsg::ref_ptr<vsg::MatrixTransform> model;
    std::vector<Mesh> meshes;
    Mesh* current_mesh = nullptr;

    vsg::ref_ptr<vsg::vec3Array>   temp_vertices;
    vsg::ref_ptr<vsg::ushortArray> temp_indices;

    std::size_t vertices_count = 0;
    std::size_t faces_count = 0;
    std::size_t indices_count = 0;
    std::size_t temp_vertices_count = 0;

    while (std::getline(stream, str))
    {
        remove_CR_symbols(str);

        if (str == "New object" && !object_added)
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
        else if (str == "numverts numfaces" && !numverts_readed)
        {
            stream >> temp_vertices_count >> faces_count;
            indices_count = faces_count * 3;

            temp_vertices = vsg::vec3Array::create(temp_vertices_count);
            temp_indices = vsg::ushortArray::create(indices_count);

            numverts_readed = true;
        }
        else if (str == "Mesh vertices:" && !vertices_readed)
        {
            for (auto& tempVertex : *temp_vertices)
            {
                stream >> tempVertex;
            }

            vertices_readed = true;
        }
        else if (str == "Mesh faces:" && !faces_readed)
        {
            for (auto& tempIndex : *temp_indices)
            {
                stream >> tempIndex;
                --tempIndex;
            }

            faces_readed = true;
        }
        else if (str == "numtverts numtvfaces" && !numtverts_readed)
        {
            stream >> vertices_count >> faces_count;

            current_mesh->vertices = vsg::vec3Array::create(vertices_count);
            current_mesh->normals = vsg::vec3Array::create(vertices_count);
            current_mesh->tex_coords = vsg::vec3Array::create(vertices_count);
            current_mesh->colors = vsg::vec4Array::create(vertices_count);
            current_mesh->indices = vsg::ushortArray::create(indices_count);

            for (auto& color : *current_mesh->colors)
            {
                color.set(1.0f, 1.0f, 1.0f, 1.0f);
            }

            numtverts_readed = true;
        }
        else if (str == "Texture vertices:" && !tverts_readed)
        {
            for (auto& texCoord : *current_mesh->tex_coords)
            {
                stream >> texCoord;
            }

            tverts_readed = true;
        }
        else if (str == "Texture faces:" && !tfaces_readed)
        {
            std::set<std::size_t> processed_indices;
            for (std::size_t i = 0; i < indices_count; ++i)
            {
                auto& index = current_mesh->indices->at(i);
                stream >> index;
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
        return vsg::ref_ptr<vsg::MatrixTransform>();
    }

    temp_vertices.reset();
    temp_indices.reset();
    vertices_count = 0;
    faces_count = 0;
    indices_count = 0;
    temp_vertices_count = 0;

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
    model_data->tex_coords = vsg::vec3Array::create(model_vertices_count);
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

void DMD_Reader::remove_CR_symbols(std::string& str) const
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
