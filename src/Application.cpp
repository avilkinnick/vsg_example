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

Application::~Application()
{
    // !!!
    // std::cout << "Dtor\n";
}

void Application::run()
{
    initialize();
    main_loop();
}

void Application::initialize()
{
    initialize_options();
    initialize_window();
    initialize_camera();
    initialize_scene_graph();
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
    options->add(DMD_Reader::create());
}

void Application::initialize_scene_graph()
{
    const std::string route_path = "../routes/konotop-suchinichi";

    scene_graph = vsg::Group::create();

    load_objects_ref(route_path);
    load_route_map(route_path);
}

void Application::load_objects_ref(const std::string& route_path)
{
    std::set<std::string> invalid_textures;
    bool mipmap = false;
    bool smooth = false;

    std::ifstream file(route_path + "/objects.ref");
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == ':')
        {
            continue;
        }
        else if (line == "[mipmap]")
        {
            mipmap = true;
        }
        else if (line == "[not_mipmap]")
        {
            mipmap = false;
        }
        else if (line == "[smooth]")
        {
            smooth = true;
        }
        else if (line == "[not_smooth]")
        {
            smooth = false;
        }
        else
        {
            std::istringstream stream(line);
            std::string label, model_path, texture_path;
            stream >> label >> model_path >> texture_path;

            model_path = route_path + model_path;
            texture_path = route_path + texture_path;

            ObjectRef object_ref;
            object_ref.label = label;
            object_ref.model_path = model_path;
            object_ref.texture_path = texture_path;
            object_ref.mipmap = mipmap;
            object_ref.smooth = smooth;
            objects_ref.push_back(object_ref);
        }
    }
}

void Application::load_route_map(const std::string& route_path)
{
    std::ifstream file(route_path + "/route1.map");
    std::string line;

    while (std::getline(file, line))
    {
        while (true)
        {
            auto CR_pos = line.find('\r');
            if (CR_pos == std::string::npos)
            {
                break;
            }
            line.erase(CR_pos);
        }

        if (line.empty() || line.back() != ';' || line[0] == ',')
        {
            continue;
        }
        else
        {
            line.pop_back();
            std::replace(line.begin(), line.end(), ',', ' ');

            std::istringstream stream(line);
            std::string label;
            vsg::dvec3 translation, rotation;
            stream >> label >> translation >> rotation;

            rotation.x = vsg::radians(rotation.x);
            rotation.y = vsg::radians(rotation.y);
            rotation.z = vsg::radians(rotation.z);

            ObjectTransformation object_transformation;
            object_transformation.reference = nullptr;
            object_transformation.translation = translation;
            object_transformation.rotation = rotation;

            for (ObjectRef& ref : objects_ref)
            {
                if (ref.label == label)
                {
                    object_transformation.reference = &ref;
                    break;
                }
            }

            if (object_transformation.reference != nullptr)
            {
                object_transformations.push_back(object_transformation);
            }
        }
    }
}

void Application::initialize_window()
{
    auto window_traits = vsg::WindowTraits::create();
    window_traits->debugLayer = true;

    window = vsg::Window::create(window_traits);
    if (!window)
    {
        throw std::runtime_error("Failed to create window!");
    }
}

void Application::initialize_camera()
{
    const VkExtent2D& extent = window->extent2D();

    constexpr double FOV = 60.0;
    const double aspect_ratio = static_cast<double>(extent.width) / static_cast<double>(extent.height);
    constexpr double near_far_ratio = 0.001;

    vsg::dvec3 center = vsg::dvec3(0.0, 0.0, 0.0);
    double radius = 1000.0;

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
    for (const ObjectTransformation& transformation : object_transformations)
    {
        const std::string paths = transformation.reference->model_path + transformation.reference->texture_path;

        const vsg::dvec3& translation = transformation.translation;
        const vsg::dvec3& rotation = transformation.rotation;

        vsg::dmat4 m1 = vsg::translate(translation);
        vsg::dmat4 m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
        vsg::dmat4 m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
        vsg::dmat4 m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));

        auto model = vsg::read_cast<vsg::StateGroup>(paths, options);
        if (!model)
        {
            continue;
        }

        vsg::ComputeBounds bounds;
        model->accept(bounds);

        vsg::dvec3 center = (bounds.bounds.min + bounds.bounds.max) * 0.5;
        double radius = 200.0;

        auto paged_lod = vsg::PagedLOD::create();
        paged_lod->options = options;
        paged_lod->filename = paths;
        paged_lod->children[0].minimumScreenHeightRatio = 0.5;
        paged_lod->bound = vsg::dsphere(center, radius);

        auto matrix_transform = vsg::MatrixTransform::create();
        matrix_transform->matrix = m1 * m2 * m3 * m4;
        matrix_transform->addChild(paged_lod);

        scene_graph->addChild(matrix_transform);
    }

    DMD_Reader::models_loaded = true;

    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({command_graph});
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    auto resource_hints = vsg::ResourceHints::create();
    resource_hints->numDescriptorSets = 2;

    viewer->compile(resource_hints);
}
