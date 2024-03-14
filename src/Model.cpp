#include "Model.h"

#include "io.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>
#include <sstream>

Model Model::loadDmd(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> options)
{
    Model model;

    auto reader = vsg::txt::create();
    reader->supportedExtensions.insert(".dmd");

    auto stringValue = reader->read_cast<vsg::stringValue>(path, options);
    if (!stringValue)
    {
        std::cerr << "Failed to open \"" << path.c_str() << "\" for reading!\n";
    }
    else
    {
        std::stringstream stream(stringValue->value());

        // Указатель на текущий меш
        Mesh* mesh = nullptr;

        // Временные массивы для хранения координат вершин и индексов модели
        // до их привязки к текстурным координатам и индексам
        vsg::ref_ptr<vsg::vec3Array> tempVertices;
        vsg::ref_ptr<vsg::ushortArray> tempIndices;

        std::size_t verticesCount;
        std::size_t facesCount;
        std::size_t indicesCount;

        std::size_t tempVerticesCount;

        std::string input;
        while (std::getline(stream, input))
        {
            ani::remove_CR_symbols(input);

            if (input == "New object")
            {
                mesh = &(model.meshes_.emplace_back(Mesh()));
                tempVertices.reset();
                tempIndices.reset();
                verticesCount = 0;
                indicesCount = 0;
                tempVerticesCount = 0;
            }
            else if (input == "numverts numfaces")
            {
                stream >> tempVerticesCount >> facesCount;
                indicesCount = facesCount * 3;
            }
            else if (input == "Mesh vertices:")
            {
                tempVertices = vsg::vec3Array::create(tempVerticesCount);
                for (std::size_t i = 0; i < tempVerticesCount; ++i)
                {
                    stream >> tempVertices->at(i);
                }
            }
            else if (input == "Mesh faces:")
            {
                tempIndices = vsg::ushortArray::create(indicesCount);
                for (std::size_t i = 0; i < indicesCount; ++i)
                {
                    std::size_t index;
                    stream >> index;
                    tempIndices->at(i) = index - 1;
                }
            }
            else if (input == "numtverts numtvfaces")
            {
                stream >> verticesCount >> facesCount;
            }
            else if (input == "Texture vertices:")
            {
                mesh->texCoords = vsg::vec3Array::create(verticesCount);
                mesh->colors = vsg::vec4Array::create(verticesCount);
                for (std::size_t i = 0; i < verticesCount; ++i)
                {
                    stream >> mesh->texCoords->at(i);

                    mesh->colors->at(i).set(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
            else if (input == "Texture faces:")
            {
                mesh->indices = vsg::ushortArray::create(indicesCount);
                mesh->vertices = vsg::vec3Array::create(verticesCount);

                std::set<std::size_t> processedIndices;
                for (std::size_t i = 0; i < indicesCount; ++i)
                {
                    std::size_t index;
                    stream >> index;
                    mesh->indices->at(i) = --index;

                    // Если вершина с заданным индексом еще не была обработана,
                    // привязать координаты этой вершины
                    // к текстурным координатам
                    if (processedIndices.find(index) == processedIndices.end())
                    {
                        std::size_t tempIndex = tempIndices->at(i);
                        mesh->vertices->at(index) = tempVertices->at(tempIndex);
                        processedIndices.insert(index);
                    }
                }

                // Посчитать нормали вершин
                mesh->normals = vsg::vec3Array::create(verticesCount);
                for (std::size_t i = 0; i < facesCount; ++i)
                {
                    auto index1 = mesh->indices->at(i * 3);
                    auto index2 = mesh->indices->at(i * 3 + 1);
                    auto index3 = mesh->indices->at(i * 3 + 2);

                    auto& v1 = mesh->vertices->at(index1);
                    auto& v2 = mesh->vertices->at(index2);
                    auto& v3 = mesh->vertices->at(index3);

                    auto& n1 = mesh->normals->at(index1);
                    auto& n2 = mesh->normals->at(index2);
                    auto& n3 = mesh->normals->at(index3);

                    // Векторы a и b - 2 стороны полигона
                    vsg::vec3 a = v2 - v1;
                    vsg::vec3 b = v3 - v1;

                    // Вектор c - это нормальнь поверхности
                    vsg::vec3 faceNormal = vsg::cross(a, b);

                    // Добавить к каждой нормали вершины нормаль поверхности,
                    // на которой она лежит
                    n1 += faceNormal;
                    n2 += faceNormal;
                    n3 += faceNormal;
                }
            }
        }
    }

    return model;
}


