#include "Application.h"

#include "DMD_Reader.h"

#include <iostream>
#include <stdexcept>

vsg::ref_ptr<vsg::MatrixTransform> Model(
    vsg::ref_ptr<ModelData> model_data,
    vsg::ref_ptr<vsg::Data> texture_data,
    vsg::ref_ptr<const vsg::Options> options = {})
{
    if (!model_data)
    {
        return {};
    }

    auto shader_set = vsg::createPhongShaderSet(options);
    if (!shader_set)
    {
        std::cerr << "Failed to create Phong shader set!\n";
        return {};
    }

    auto pipeline = vsg::GraphicsPipelineConfigurator::create(shader_set);

    vsg::DataList vertex_arrays;
    pipeline->assignArray(vertex_arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, model_data->vertices);
    pipeline->assignArray(vertex_arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, model_data->normals);
    pipeline->assignArray(vertex_arrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, model_data->tex_coords);
    pipeline->assignArray(vertex_arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, model_data->colors);

    auto draw_commands = vsg::Commands::create();
    draw_commands->addChild(vsg::BindVertexBuffers::create(pipeline->baseAttributeBinding, vertex_arrays));
    draw_commands->addChild(vsg::BindIndexBuffer::create(model_data->indices));
    draw_commands->addChild(vsg::DrawIndexed::create(model_data->indices->size(), 1, 0, 0, 0));

    if (texture_data)
    {
        pipeline->assignTexture("diffuseMap", texture_data);
    }

    pipeline->init();

    auto state_group = vsg::StateGroup::create();
    pipeline->copyTo(state_group);
    state_group->addChild(draw_commands);

    auto matrix_transform = vsg::MatrixTransform::create();
    matrix_transform->addChild(state_group);

    return matrix_transform;
}


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
    options->add(vsgXchange::all::create());
    options->add(DMD_Reader::create());
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

        ModelData2 model_data{model_path, texture_path};
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
    vsg::LoadPagedLOD load_paged_lod(camera);

    for (auto& model_data_pair : model_map)
    {
        auto& model_data = model_data_pair.second;

        auto model_d = vsg::read_cast<ModelData>(model_data.model_path, options);

        vsg::ref_ptr<vsg::Data> texture_data;
        if (texture_map.find(model_data.texture_path) != texture_map.end())
        {
            texture_data = texture_map.at(model_data.texture_path);
        }

        auto model = Model(model_d, texture_data, options);
        if (model)
        {
            const auto& rotation = model_data.rotation;
            auto m1 = vsg::translate(model_data.translation);
            auto m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
            auto m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
            auto m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));
            model->transform(m1 * m2 * m3 * m4);

            auto paged_lod = vsg::PagedLOD::create();
            paged_lod->options = options;
            paged_lod->children[0].minimumScreenHeightRatio = 1.0;
            paged_lod->children[0].node = model;

            vsg::ComputeBounds test_bounds;
            model->accept(test_bounds);

            const vsg::dbox& bounds = test_bounds.bounds;

            vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
            double radius = vsg::length(bounds.max - bounds.min) * 0.6;

            paged_lod->bound = vsg::dsphere(center, radius);

            // std::cout << paged_lod->bound << '\n';

            load_paged_lod.apply(*(paged_lod.get()));

            scene_graph->addChild(paged_lod);
            if (scene_graph->children.size() > 500)
            {
                break;
            }
        }

    }
    scene_graph->accept(load_paged_lod);

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

    // vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
    // double radius = vsg::length(bounds.max - bounds.min) * 0.6;
    vsg::dvec3 center = vsg::dvec3(0.0, 0.0, 0.0);
    double radius = 1000.0;
    // std::cout << center << ' ' << radius << '\n';

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
