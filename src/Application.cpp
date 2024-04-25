#include "Application.h"

#include "DMD_Reader.h"

#include <iostream>
#include <stdexcept>

//------------------------------------------------------------------------------

class Model
{
public:
    vsg::ref_ptr<vsg::Data> texture_data;
    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> pipeline;
    vsg::ref_ptr<vsg::MatrixTransform> transform;
    vsg::ref_ptr<vsg::PagedLOD> paged_lod;
    bool visible = false;

    void show()
    {
        if (!visible)
        {
            pipeline->assignTexture("diffuseMap", texture_data);
            visible = true;
        }
    }

    void hide()
    {
        if (visible)
        {
            pipeline->assignTexture("diffuseMap");
            visible = false;
        }
    }
};

std::vector<Model> models;

bool add_model(vsg::ref_ptr<ModelData> vertex_data, vsg::ref_ptr<vsg::Data> texture_data, vsg::ref_ptr<const vsg::Options> options = {})
{
    if (!vertex_data)
    {
        return false;
    }

    static auto shader_set = vsg::createPhongShaderSet(options);
    if (!shader_set)
    {
        std::cerr << "Failed to create Phong shader set!\n";
        return false;
    }

    Model model;

    model.texture_data = texture_data;
    model.pipeline = vsg::GraphicsPipelineConfigurator::create(shader_set);

    vsg::DataList vertex_arrays;
    model.pipeline->assignArray(vertex_arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, vertex_data->vertices);
    model.pipeline->assignArray(vertex_arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, vertex_data->normals);
    model.pipeline->assignArray(vertex_arrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, vertex_data->tex_coords);
    model.pipeline->assignArray(vertex_arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, vertex_data->colors);

    auto draw_commands = vsg::Commands::create();
    draw_commands->addChild(vsg::BindVertexBuffers::create(model.pipeline->baseAttributeBinding, vertex_arrays));
    draw_commands->addChild(vsg::BindIndexBuffer::create(vertex_data->indices));
    draw_commands->addChild(vsg::DrawIndexed::create(vertex_data->indices->size(), 1, 0, 0, 0));

    if (texture_data)
    {
        model.show();
    }

    model.pipeline->init();

    auto state_group = vsg::StateGroup::create();
    model.pipeline->copyTo(state_group);
    state_group->addChild(draw_commands);

    model.transform = vsg::MatrixTransform::create();
    model.transform->addChild(state_group);

    models.push_back(model);

    return true;
}

//------------------------------------------------------------------------------

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
    const std::string route_path = "../routes/konotop-suchinichi";

    scene_graph = vsg::Group::create();

    load_objects_ref(route_path);
    load_route_map(route_path);
    add_models_to_scene_graph();

    scene_graph->accept(compute_bounds);
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

            bool texture_already_loaded = (texture_map.find(texture_path) != texture_map.end());
            if (texture_already_loaded)
            {
                continue;
            }

            bool texture_is_invalid = (invalid_textures.find(texture_path) != invalid_textures.end());
            if (texture_is_invalid)
            {
                texture_map.insert({texture_path, {}});

                continue;
            }

            auto texture_data = vsg::read_cast<vsg::Data>(texture_path, options);
            if (!texture_data)
            {
                invalid_textures.insert(texture_path);
                std::cerr << "Failed to read \"" << texture_path << "\"\n";
                texture_map.insert({texture_path, {}});
            }
            else
            {
                texture_map.insert({texture_path, texture_data});
            }
        }
    }

    //--------------------------------------------------------------------------
    std::ofstream temp1("../temp1.txt");
    const size_t length = objects_ref.size();
    for (size_t i = 0; i < length; ++i)
    {
        ObjectRef& object_ref = objects_ref[i];
        temp1 << "Object " << i << ":\n";
        temp1 << "    label:        " << object_ref.label << '\n';
        temp1 << "    model_path:   " << object_ref.model_path << '\n';
        temp1 << "    texture_path: " << object_ref.texture_path << '\n';
        temp1 << "    mipmap:       " << object_ref.mipmap << '\n';
        temp1 << "    smooth:       " << object_ref.smooth << '\n';
        temp1 << '\n';
    }
    temp1.close();

    std::ofstream temp2("../temp2.txt");
    size_t i = 0;
    for (auto& [texture_path, texture_data] : texture_map)
    {
        temp2 << "Texture " << i++ << ":\n";
        temp2 << "    path: " << texture_path << '\n';
        temp2 << "    data: " << texture_data << '\n';
        temp2 << '\n';
    }
    temp2.close();
    //--------------------------------------------------------------------------
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
            object_transformation.label = label;
            object_transformation.translation = translation;
            object_transformation.rotation = rotation;

            object_transformations.push_back(object_transformation);
        }
    }

    //--------------------------------------------------------------------------
    std::ofstream temp3("../temp3.txt");
    const size_t length = object_transformations.size();
    for (size_t i = 0; i < length; ++i)
    {
        ObjectTransformation& transformation = object_transformations[i];
        temp3 << "Object transformation " << i << ":\n";
        temp3 << "    label:       " << transformation.label << '\n';
        temp3 << "    translation: " << transformation.translation << '\n';
        temp3 << "    rotation:    " << transformation.rotation << '\n';
        temp3 << '\n';
    }
    temp3.close();
    //--------------------------------------------------------------------------
}

void Application::add_models_to_scene_graph()
{
    vsg::LoadPagedLOD load_paged_lod(camera);

    for (const ObjectTransformation& object_transformation : object_transformations)
    {
        const ObjectRef* object_ref = nullptr;
        for (const ObjectRef& ref : objects_ref)
        {
            if (ref.label == object_transformation.label)
            {
                object_ref = &ref;
            }
        }

        if (!object_ref)
        {
            continue;
        }

        auto model_data = vsg::read_cast<ModelData>(object_ref->model_path, options);
        if (!model_data)
        {
            continue;
        }

        vsg::ref_ptr<vsg::Data> texture_data;
        if (texture_map.find(object_ref->texture_path) != texture_map.end())
        {
            texture_data = texture_map.at(object_ref->texture_path);
        }

        if (!add_model(model_data, texture_data, options))
        {
            continue;
        }

        auto model = models.back();

        if (!add_model(model_data, {}, options))
        {
            continue;
        }

        auto model2 = models.back();

        const vsg::dvec3& translation = object_transformation.translation;
        const vsg::dvec3& rotation = object_transformation.rotation;

        vsg::dmat4 m1 = vsg::translate(translation);
        vsg::dmat4 m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
        vsg::dmat4 m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
        vsg::dmat4 m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));
        model.transform->matrix = m1 * m2 * m3 * m4;
        model2.transform->matrix = m1 * m2 * m3 * m4;

        model.paged_lod = vsg::PagedLOD::create();
        model.paged_lod->children[0].minimumScreenHeightRatio = 1.0;
        model.paged_lod->children[0].node = model.transform;
        model.paged_lod->children[1].minimumScreenHeightRatio = 0.0;
        model.paged_lod->children[1].node = model2.transform;

        vsg::ComputeBounds test_bounds;
        model.transform->accept(test_bounds);
        const vsg::dbox& bounds = test_bounds.bounds;
        vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
        double radius = vsg::length(bounds.max - bounds.min) * 0.6;

        model.paged_lod->bound = vsg::dsphere(center, radius);

        load_paged_lod.apply(*model.paged_lod.get());

        scene_graph->addChild(model.paged_lod);

        if (scene_graph->children.size() > 500)
        {
            break;
        }
    }

    scene_graph->accept(load_paged_lod);

    std::cout << "Loaded " << scene_graph->children.size() << " children\n";

    models.clear();
    object_transformations.clear();
    texture_map.clear();
    objects_ref.clear();
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
    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({command_graph});
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();
}
