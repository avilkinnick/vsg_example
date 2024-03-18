#include "DMD_Reader.h"

#include <iostream>

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

    model_ = Model::create();

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
            std::set<std::size_t> processedIndices;
            std::size_t indicesCount = currentMesh_->indices->size();

            for (std::size_t i = 0; i < indicesCount; ++i)
            {
                std::size_t index;
                stream >> index;
                --index;
                currentMesh_->indices->at(i) = index;

                // Если вершина с заданным индексом еще не была обработана,
                // привязать координаты этой вершины
                // к текстурным координатам
                if (processedIndices.find(index) == processedIndices.end())
                {
                    std::size_t tempIndex = tempIndices_->at(i);
                    currentMesh_->vertices->at(index) = tempVertices_->at(tempIndex);
                    processedIndices.insert(index);
                }
            }

            // Посчитать нормали вершин
            std::size_t facesCount = currentMesh_->indices->size() / 3;
            for (std::size_t i = 0; i < facesCount; ++i)
            {
                auto index1 = currentMesh_->indices->at(i * 3);
                auto index2 = currentMesh_->indices->at(i * 3 + 1);
                auto index3 = currentMesh_->indices->at(i * 3 + 2);

                auto& v1 = currentMesh_->vertices->at(index1);
                auto& v2 = currentMesh_->vertices->at(index2);
                auto& v3 = currentMesh_->vertices->at(index3);

                auto& n1 = currentMesh_->normals->at(index1);
                auto& n2 = currentMesh_->normals->at(index2);
                auto& n3 = currentMesh_->normals->at(index3);

                // Вектор c - это нормальнь поверхности
                vsg::vec3 faceNormal = vsg::cross(v2 - v1, v3 - v1);

                // Добавить к каждой нормали вершины нормаль поверхности,
                // на которой она лежит
                n1 += faceNormal;
                n2 += faceNormal;
                n3 += faceNormal;
            }
        }
    }
    return model_;
}

void DMD_Reader::remove_CR_symbols(std::string& str)
{
    while (true)
    {
        auto CR_pos = str.find('\r');
        if (CR_pos == std::string::npos) break;
        str.erase(CR_pos);
    }
}

void DMD_Reader::add_mesh()
{
    currentMesh_ = &(model_->meshes.emplace_back(Mesh()));
    tempVertices_.reset();
    tempIndices_.reset();
}

void DMD_Reader::read_numverts_and_numfaces(std::stringstream& stream)
{
    stream >> tempVerticesCount_ >> facesCount_;
}

void DMD_Reader::init_temp_arrays()
{
    tempVertices_ = vsg::vec3Array::create(tempVerticesCount_);
    tempIndices_ = vsg::ushortArray::create(facesCount_ * 3);
}

void DMD_Reader::read_mesh_vertices(std::stringstream& stream)
{
    for (auto& tempVertex : *tempVertices_)
        stream >> tempVertex;
}

void DMD_Reader::read_mesh_faces(std::stringstream& stream)
{
    for (auto& tempIndex : *tempIndices_)
    {
        stream >> tempIndex;
        --tempIndex;
    }
}

void DMD_Reader::read_numtverts_and_numtvfaces(std::stringstream& stream)
{
    stream >> verticesCount_ >> facesCount_;
}

void DMD_Reader::init_mesh_arrays()
{
    currentMesh_->vertices = vsg::vec3Array::create(verticesCount_);
    currentMesh_->normals = vsg::vec3Array::create(verticesCount_);
    currentMesh_->texCoords = vsg::vec3Array::create(verticesCount_);
    currentMesh_->colors = vsg::vec4Array::create(verticesCount_);
    currentMesh_->indices = vsg::ushortArray::create(facesCount_ * 3);
}

void DMD_Reader::read_texture_vertices(std::stringstream& stream)
{
    for (auto& texCoord : *currentMesh_->texCoords)
    {
        stream >> texCoord;
    }
}

void DMD_Reader::set_default_color()
{
    for (auto& color : *currentMesh_->colors)
    {
        color.set(1.0f, 1.0f, 1.0f, 1.0f);
    }
}
