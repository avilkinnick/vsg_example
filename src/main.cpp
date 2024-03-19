#include "DMD_Reader.h"
#include "vsg_Instance.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
    // Конструктор vsg_Instance выполняет начальную инициализацию
    vsg_Instance vsg_instance(&argc, argv);

    // Загрузить модель
    auto model = DMD_Reader().read(vsg_instance.arguments[1], vsg_instance.options);
    auto& mesh = model->meshes[0];

    // Загрузить текстуру
    // ! ТЕКСТУРА ДОЛЖНА БЫТЬ ЗАРАНЕЕ ОТРАЖЕНА ПО ВЕРТИКАЛИ !
    vsg::Path texture_path(vsg_instance.arguments[2]);
    if (texture_path)
    {
        auto texture_data = vsg::read_cast<vsg::Data>(texture_path, vsg_instance.options);
        if (!texture_data)
        {
            std::cerr << "Failed to read texture file: " << texture_path << '\n';
        }
        else
        {
            vsg_instance.graphicsPipelineConfig->assignTexture("diffuseMap", texture_data);
        }
    }

    // Передать модель в графический конвейер
    vsg::DataList vertexArrays;
    vsg_instance.graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, mesh.vertices);
    vsg_instance.graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, mesh.normals);
    vsg_instance.graphicsPipelineConfig->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, mesh.tex_coords);
    vsg_instance.graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, mesh.colors);

    vsg_instance.drawCommands->addChild(vsg::BindVertexBuffers::create(vsg_instance.graphicsPipelineConfig->baseAttributeBinding, vertexArrays));
    vsg_instance.drawCommands->addChild(vsg::BindIndexBuffer::create(mesh.indices));
    vsg_instance.drawCommands->addChild(vsg::DrawIndexed::create(mesh.indices->size(), 1, 0, 0, 0));

    // После передачи модели инициализировать все остальное и запустить приложение
    vsg_instance.run();

    return EXIT_SUCCESS;
}

