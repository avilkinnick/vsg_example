#include "Model.h"

#include "io.h"

#include <fstream>
#include <iostream>
#include <set>


#include <vsg/all.h>
#include <vsgXchange/all.h>


Model Model::loadDmd(const vsg::Path& path)
{
    Model model;

    auto options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->add(vsgXchange::all::create());

    auto file2 = vsg::read(path, options);
    std::string test;

    std::ifstream file(path.c_str());
    if (!file)
    {
        std::cerr << "Failed to open \"" << path.c_str() << "\" for reading!\n";
    }
    else
    {
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
        while (std::getline(file, input))
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
                file >> tempVerticesCount >> facesCount;
                indicesCount = facesCount * 3;
            }
            else if (input == "Mesh vertices:")
            {
                tempVertices = vsg::vec3Array::create(tempVerticesCount);
                for (std::size_t i = 0; i < tempVerticesCount; ++i)
                {
                    auto& vertex = tempVertices->at(i);
                    file >> vertex.x >> vertex.y >> vertex.z;
                }
            }
            else if (input == "Mesh faces:")
            {
                tempIndices = vsg::ushortArray::create(indicesCount);
                for (std::size_t i = 0; i < indicesCount; ++i)
                {
                    std::size_t index;
                    file >> index;
                    tempIndices->at(i) = index - 1;
                }
            }
            else if (input == "numtverts numtvfaces")
            {
                file >> verticesCount >> facesCount;
            }
            else if (input == "Texture vertices:")
            {
                mesh->texCoords = vsg::vec3Array::create(verticesCount);
                mesh->colors = vsg::vec4Array::create(verticesCount);
                for (std::size_t i = 0; i < verticesCount; ++i)
                {
                    auto& texCoord = mesh->texCoords->at(i);
                    file >> texCoord.x >> texCoord.y >> texCoord.z;

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
                    file >> index;
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
                    vsg::vec3 a(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z);
                    vsg::vec3 b(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z);

                    // Вектор {x, y, z} - это векторное произведение a и b,
                    // он представляет нормаль поверхности
                    auto x = a.y * b.z - a.z * b.y;
                    auto y = a.z * b.x - a.x * b.z;
                    auto z = a.x * b.y - a.y * b.x;

                    // Добавить к каждой нормали вершины нормаль поверхности,
                    // на которой она лежит
                    n1.set(n1.x + x, n1.y + y, n1.z + z);
                    n2.set(n2.x + x, n2.y + y, n2.z + z);
                    n3.set(n3.x + x, n3.y + y, n3.z + z);
                }
            }
        }
    }

    return model;
}


