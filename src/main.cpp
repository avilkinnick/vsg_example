#include "DMD_Reader.h"
#include "vsg_Instance.h"

int main(int argc, char* argv[])
{
    // Конструктор vsg_Instance выполняет начальную инициализацию
    vsg_Instance vsg_instance(&argc, argv);

    // Загрузить модель
    auto model1 = DMD_Reader().read(vsg_instance.arguments[1],
                                   vsg_instance.arguments[2],
                                   vsg_instance.options);

    vsg_instance.sceneGraph->addChild(model1);

    auto model2 = DMD_Reader().read(vsg_instance.arguments[3],
                                    vsg_instance.arguments[4],
                                    vsg_instance.options);

    vsg_instance.sceneGraph->addChild(model2);

    // После передачи модели инициализировать все остальное и запустить приложение
    vsg_instance.run();

    return 0;
}

