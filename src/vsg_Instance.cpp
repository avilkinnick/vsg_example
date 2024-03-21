#include "vsg_Instance.h"

#include <iostream>

vsg_Instance::vsg_Instance(int* argc, char** argv)
    : arguments(argc, argv)
{
    options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->add(vsgXchange::all::create());

    windowTraits = vsg::WindowTraits::create();

    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cerr);
        return;
    }

    sceneGraph = vsg::Group::create();

    window = vsg::Window::create(windowTraits);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        return;
    }

    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
}

void vsg_Instance::run()
{
    vsg::ComputeBounds computeBounds;
    sceneGraph->accept(computeBounds);
    vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;

    double nearFarRatio = 0.001;
    auto viewport = vsg::ViewportState::create(0, 0, window->extent2D().width, window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(10.0, 10.0, 10.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    camera = vsg::Camera::create(perspective, lookAt, viewport);

    auto commandGraph = vsg::createCommandGraphForView(window, camera, sceneGraph);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->compile();
    viewer->addEventHandlers({vsg::CloseHandler::create(viewer)});
    viewer->addEventHandler(vsg::Trackball::create(camera));

    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
}
