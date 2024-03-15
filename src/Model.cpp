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
        Mesh* currentMesh = nullptr;

        // Временные массивы для хранения координат вершин и индексов модели
        // до их привязки к текстурным координатам и индексам
        vsg::ref_ptr<vsg::vec3Array> tempVertices;
        vsg::ref_ptr<vsg::ushortArray> tempIndices;

        std::string input;
        while (std::getline(stream, input))
        {
            ani::remove_CR_symbols(input);

            if (input == "New object")
            {
                currentMesh = &(model.meshes_.emplace_back(Mesh()));
                tempVertices.reset();
                tempIndices.reset();
            }
            else if (input == "numverts numfaces")
            {
                std::size_t tempVerticesCount;
                std::size_t facesCount;

                stream >> tempVerticesCount >> facesCount;

                tempVertices = vsg::vec3Array::create(tempVerticesCount);
                tempIndices = vsg::ushortArray::create(facesCount * 3);
            }
            else if (input == "Mesh vertices:")
            {
                for (auto& tempVertex : *tempVertices)
                {
                    stream >> tempVertex;
                }
            }
            else if (input == "Mesh faces:")
            {
                for (auto& tempIndex : *tempIndices)
                {
                    stream >> tempIndex;
                    --tempIndex;
                }
            }
            else if (input == "numtverts numtvfaces")
            {
                std::size_t verticesCount;
                std::size_t facesCount;

                stream >> verticesCount >> facesCount;

                currentMesh->vertices = vsg::vec3Array::create(verticesCount);
                currentMesh->normals = vsg::vec3Array::create(verticesCount);
                currentMesh->texCoords = vsg::vec3Array::create(verticesCount);
                currentMesh->colors = vsg::vec4Array::create(verticesCount);
                currentMesh->indices = vsg::ushortArray::create(facesCount * 3);
            }
            else if (input == "Texture vertices:")
            {
                for (auto& texCoord : *currentMesh->texCoords)
                {
                    stream >> texCoord;
                }

                for (auto& color : *currentMesh->colors)
                {
                    color.set(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
            else if (input == "Texture faces:")
            {
                std::set<std::size_t> processedIndices;
                std::size_t indicesCount = currentMesh->indices->size();

                for (std::size_t i = 0; i < indicesCount; ++i)
                {
                    std::size_t index;
                    stream >> index;
                    --index;
                    currentMesh->indices->at(i) = index;

                    // Если вершина с заданным индексом еще не была обработана,
                    // привязать координаты этой вершины
                    // к текстурным координатам
                    if (processedIndices.find(index) == processedIndices.end())
                    {
                        std::size_t tempIndex = tempIndices->at(i);
                        currentMesh->vertices->at(index) = tempVertices->at(tempIndex);
                        processedIndices.insert(index);
                    }
                }

                // Посчитать нормали вершин
                std::size_t facesCount = currentMesh->indices->size() / 3;
                for (std::size_t i = 0; i < facesCount; ++i)
                {
                    auto index1 = currentMesh->indices->at(i * 3);
                    auto index2 = currentMesh->indices->at(i * 3 + 1);
                    auto index3 = currentMesh->indices->at(i * 3 + 2);

                    auto& v1 = currentMesh->vertices->at(index1);
                    auto& v2 = currentMesh->vertices->at(index2);
                    auto& v3 = currentMesh->vertices->at(index3);

                    auto& n1 = currentMesh->normals->at(index1);
                    auto& n2 = currentMesh->normals->at(index2);
                    auto& n3 = currentMesh->normals->at(index3);

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
    }

    return model;
}
