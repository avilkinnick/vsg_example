#include "DMD_Reader.h"

#include <vsg/core/Value.h>
#include <vsg/maths/vec3.h>

#include <iostream>
#include <set>
#include <string>

DMD_Reader::DMD_Reader()
{
    supportedExtensions.insert(".dmd");
}

vsg::ref_ptr<Model> DMD_Reader::read(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> options)
{
    auto stringValue = read_cast<vsg::stringValue>(path, options);
    if (!stringValue)
    {
        std::cerr << "Failed to open \"" << path << "\" for reading\n";
        return vsg::ref_ptr<Model>();
    }

    std::stringstream stream(stringValue->value());

    m_model = Model::create();

    std::string input;
    while (std::getline(stream, input))
    {
        remove_CR_symbols(input);

        if (input == "New object")
        {
            add_mesh();
        }
        else if (input == "numverts numfaces")
        {
            read_numverts_and_numfaces(stream);
            init_temp_arrays();
        }
        else if (input == "Mesh vertices:")
        {
            read_mesh_vertices(stream);
        }
        else if (input == "Mesh faces:")
        {
            read_mesh_faces(stream);
        }
        else if (input == "numtverts numtvfaces")
        {
            read_numtverts_and_numtvfaces(stream);
            init_mesh_arrays();
            set_default_color();
        }
        else if (input == "Texture vertices:")
        {
            read_texture_vertices(stream);
        }
        else if (input == "Texture faces:")
        {
            read_and_proceed_texture_faces(stream);
            calculate_vertex_normals();
        }
    }
    return m_model;
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
    m_current_mesh = &(m_model->meshes.emplace_back(Mesh()));
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
        auto index1 = m_current_mesh->indices->at(i * 3);
        auto index2 = m_current_mesh->indices->at(i * 3 + 1);
        auto index3 = m_current_mesh->indices->at(i * 3 + 2);

        auto& vertex1 = m_current_mesh->vertices->at(index1);
        auto& vertex2 = m_current_mesh->vertices->at(index2);
        auto& vertex3 = m_current_mesh->vertices->at(index3);

        auto& vertex_normal1 = m_current_mesh->normals->at(index1);
        auto& vertex_normal2 = m_current_mesh->normals->at(index2);
        auto& vertex_normal3 = m_current_mesh->normals->at(index3);

        vsg::vec3 face_normal = vsg::cross(vertex2 - vertex1, vertex3 - vertex1);

        // Добавить к каждой нормали вершины нормаль поверхности,
        // на которой она лежит
        vertex_normal1 += face_normal;
        vertex_normal2 += face_normal;
        vertex_normal3 += face_normal;
    }
}
