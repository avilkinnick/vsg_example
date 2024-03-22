#include "DMD_Reader.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>

struct ModelPaths
{
    std::string model_path;
    std::string texture_path;
};

int main(int argc, char* argv[])
{
    auto options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->add(vsgXchange::all::create());

    vsg::CommandLine arguments(&argc, argv);
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cerr);
        return -1;
    }



    std::string route_path = "../routes/konotop-suchinichi/";
    std::map<std::string, ModelPaths> model_paths_map;
    std::ifstream file(route_path + "objects.ref");
    std::string str;
    while (std::getline(file, str))
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



    // Загрузить модель
    auto model1 = DMD_Reader().read(arguments[1], arguments[2], options);

    auto model2 = DMD_Reader().read(arguments[3], arguments[4], options);
    model2->matrix = vsg::translate(0.0f, 0.0f, 20.0f) * vsg::scale(0.2f) * vsg::rotate(vsg::radians(10.0f), 0.0f, 0.0f, 1.0f);

    auto scene_graph = vsg::Group::create();
    scene_graph->addChild(model1);
    scene_graph->addChild(model2);

    vsg::ComputeBounds compute_bounds;
    scene_graph->accept(compute_bounds);

    vsg::dvec3 centre = (compute_bounds.bounds.min + compute_bounds.bounds.max) * 0.5;
    double radius = vsg::length(compute_bounds.bounds.max - compute_bounds.bounds.min) * 0.6;

    auto window = vsg::Window::create(vsg::WindowTraits::create());
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        return -1;
    }

    double near_far_ratio = 0.001;
    auto window_width = static_cast<double>(window->extent2D().width);
    auto window_height = static_cast<double>(window->extent2D().height);
    auto viewport = vsg::ViewportState::create(0, 0, window_width, window_height);
    auto perspective = vsg::Perspective::create(60.0, window_width / window_height, near_far_ratio * radius, radius * 10.0);
    auto look_at = vsg::LookAt::create(centre + vsg::dvec3(10.0, 10.0, 10.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, look_at, viewport);

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene_graph);

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
    viewer->compile();
    viewer->addEventHandlers({vsg::CloseHandler::create(viewer)});
    viewer->addEventHandler(vsg::Trackball::create(camera));

    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();

        float time = std::chrono::duration<float, std::chrono::seconds::period>(viewer->getFrameStamp()->time - viewer->start_point()).count();
        model2->matrix = vsg::translate(0.0f, 0.0f, 20.0f) * vsg::scale((float)sin(time) / 4.0f + 0.5f) * vsg::rotate(vsg::radians(time * 100.0f), 0.0f, 0.0f, 1.0f);

        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }

    return 0;
}

