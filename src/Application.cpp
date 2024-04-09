#include "Application.h"

#include "DMD_Reader.h"

#include <iostream>
#include <stdexcept>

Application::Application(int* argc, char** argv)
    : arguments(argc, argv)
{
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cerr);

        throw std::runtime_error("Failed to read arguments!");
    }
}

void Application::run()
{
    initialize();
    main_loop();
}

void Application::initialize()
{
    initialize_options();
    initialize_scene_graph();
    initialize_window();
    initialize_camera();
    initialize_command_graph();
    initialize_viewer();
}

void Application::main_loop()
{
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
}

void Application::initialize_options()
{
    options = vsg::Options::create();
    options->add(vsgXchange::all::create());
}

void Application::initialize_scene_graph()
{
    scene_graph = vsg::Group::create();

    load_model_paths();
    load_model_transformations();
    add_models_to_scene_graph();

    scene_graph->accept(compute_bounds);
}

void Application::load_model_paths()
{
    std::ifstream file(route_path + "/objects.ref");
    std::string str;

    while (std::getline(file, str))
    {
        if (str.empty() || str[0] == ';' || str[0] == '[' || str[0] == ':')
        {
            continue;
        }

        std::istringstream stream(str);
        std::string name, model_path, texture_path;
        stream >> name >> model_path >> texture_path;

        model_path = route_path + model_path;
        texture_path = route_path + texture_path;

        ModelData model_data{model_path, texture_path};
        model_map.insert({name, model_data});

        if (texture_map.find(texture_path) != texture_map.end())
        {
            continue;
        }

        auto texture_data = vsg::read_cast<vsg::Data>(texture_path, options);
        if (!texture_data)
        {
            std::cerr << "Failed to read texture file \""
                      << texture_path << "\"\n";
        }
        else
        {
            texture_map.insert({texture_path, texture_data});
        }
    }
}

void Application::load_model_transformations()
{
    std::ifstream file(route_path + "/route1.map");
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

        if (str.empty() || str.back() != ';')
        {
            continue;
        }

        str.pop_back();
        std::replace(str.begin(), str.end(), ',', ' ');

        std::stringstream stream(str);
        std::string name;
        stream >> name;

        if (model_map.find(name) != model_map.end())
        {
            auto& translation = model_map.at(name).translation;
            auto& rotation = model_map.at(name).rotation;

            stream >> translation.x >> translation.y >> translation.z
                >> rotation.x >> rotation.y >> rotation.z;

            rotation.x = vsg::radians(rotation.x);
            rotation.y = vsg::radians(rotation.y);
            rotation.z = vsg::radians(rotation.z);
        }
    }
}

void Application::add_models_to_scene_graph()
{
    for (auto& model_data_pair : model_map)
    {
        auto& model_data = model_data_pair.second;

        vsg::ref_ptr<vsg::Data> texture_data;
        if (texture_map.find(model_data.texture_path) != texture_map.end())
        {
            texture_data = texture_map.at(model_data.texture_path);
        }

        auto model = DMD_Reader().read(model_data.model_path,
                                       texture_data, options);

        if (model)
        {
            const auto& rotation = model_data.rotation;
            auto m1 = vsg::translate(model_data.translation);
            auto m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
            auto m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
            auto m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));
            model->transform(m1 * m2 * m3 * m4);

            scene_graph->addChild(model);
            if (scene_graph->children.size() > 500)
            {
                break;
            }
        }
    }

    model_map.clear();
    texture_map.clear();
}

void Application::initialize_window()
{
    window = vsg::Window::create(vsg::WindowTraits::create());

    if (!window)
    {
        throw std::runtime_error("Failed to create window!");
    }
}

void Application::initialize_camera()
{
    const vsg::dbox& bounds = compute_bounds.bounds;
    const VkExtent2D& extent = window->extent2D();

    constexpr double FOV = 60.0;
    const double aspect_ratio = static_cast<double>(extent.width) / static_cast<double>(extent.height);
    constexpr double near_far_ratio = 0.001;

    vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.6;

    vsg::dvec3 eye = center + vsg::dvec3(100.0, 100.0, 100.0);
    vsg::dvec3 up = vsg::dvec3(0.0, 0.0, 1.0);

    auto viewport = vsg::ViewportState::create(0, 0, extent.width, extent.height);
    auto perspective = vsg::Perspective::create(60.0, aspect_ratio, near_far_ratio * radius, radius * 10.0);
    auto look_at = vsg::LookAt::create(eye, center, up);

    camera = vsg::Camera::create(perspective, look_at, viewport);
}

void Application::initialize_command_graph()
{
    command_graph = vsg::createCommandGraphForView(window, camera, scene_graph);
}

void Application::initialize_viewer()
{
    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({command_graph});
    viewer->compile();
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));
}
