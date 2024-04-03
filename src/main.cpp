#include "DMD_Reader.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>

struct ModelData
{
    std::string model_path;
    std::string texture_path;
    vsg::dvec3 translation;
    vsg::dvec3 rotation;
};

const std::string route_path = "../routes/konotop-suchinichi";
std::map<std::string, ModelData> model_map;
std::map<std::string, vsg::ref_ptr<vsg::Data>> texture_map;

void load_model_paths(vsg::ref_ptr<const vsg::Options> options);
void load_model_transformations();
void add_models_to_scene_graph(vsg::ref_ptr<vsg::Group> scene_graph,
                               vsg::ref_ptr<const vsg::Options> options);

int main(int argc, char* argv[])
{
    try
    {
        auto options = vsg::Options::create();
        options->add(vsgXchange::all::create());

        vsg::CommandLine arguments(&argc, argv);
        if (arguments.errors())
        {
            arguments.writeErrorMessages(std::cerr);
            return -1;
        }

        auto scene_graph = vsg::Group::create();

        load_model_paths(options);;
        load_model_transformations();
        add_models_to_scene_graph(scene_graph, options);

        auto window = vsg::Window::create(vsg::WindowTraits::create());
        if (!window)
        {
            std::cerr << "Failed to create window\n";
            return -1;
        }

        double near_far_ratio = 0.001;
        double window_width = static_cast<double>(window->extent2D().width);
        double window_height = static_cast<double>(window->extent2D().height);
        double aspect_ratio = window_width / window_height;

        vsg::ComputeBounds compute_bounds;
        scene_graph->accept(compute_bounds);

        // vsg::dvec3 centre = (compute_boundss.bounds.min + compute_bounds.bounds.max) * 0.5;
        // double radius = vsg::length(compute_bounds.bounds.max - compute_bounds.bounds.min) * 0.6;
        vsg::dvec3 centre(0.0, 0.0, 50.0);
        double radius = 3000.0;
        std::cout << "centre = " << centre << '\n';
        std::cout << "radius = " << radius << '\n';
        auto viewport = vsg::ViewportState::create(0, 0, window_width, window_height);
        auto perspective = vsg::Perspective::create(60.0, aspect_ratio, near_far_ratio * radius, radius * 10.0);
        auto look_at = vsg::LookAt::create(centre + vsg::dvec3(100.0, 100.0, 100.0), centre + vsg::dvec3(10.0, 10.0, 10.0), vsg::dvec3(0.0, 0.0, 1.0));
        auto camera = vsg::Camera::create(perspective, look_at, viewport);

        auto command_graph = vsg::createCommandGraphForView(window, camera, scene_graph);

        auto viewer = vsg::Viewer::create();
        viewer->addWindow(window);
        viewer->assignRecordAndSubmitTaskAndPresentation({command_graph});
        viewer->compile();
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));

        auto trackball = vsg::Trackball::create(camera);
        trackball->moveForwardKey = vsg::KEY_w;
        trackball->moveBackwardKey = vsg::KEY_s;
        trackball->moveLeftKey = vsg::KEY_a;
        trackball->moveRightKey = vsg::KEY_d;
        trackball->turnLeftKey = vsg::KEY_Leftcurlybracket;
        trackball->turnRightKey = vsg::KEY_Leftcurlybracket;
        trackball->pitchUpKey = vsg::KEY_Leftcurlybracket;
        trackball->pitchDownKey = vsg::KEY_Leftcurlybracket;
        trackball->rollLeftKey = vsg::KEY_Leftcurlybracket;
        trackball->rollRightKey = vsg::KEY_Leftcurlybracket;
        trackball->moveUpKey = vsg::KEY_Leftcurlybracket;
        trackball->moveDownKey = vsg::KEY_Leftcurlybracket;

        viewer->addEventHandler(trackball);

        while (viewer->advanceToNextFrame())
        {
            viewer->handleEvents();
            viewer->update();
            viewer->recordAndSubmit();
            viewer->present();
        }
    }
    catch (const vsg::Exception& error)
    {
        std::cerr << error.message << '\n';
    }

    return 0;
}

// void handle_str(const std::string& str, vsg::ref_ptr<const vsg::Options> options)
// {
//     std::istringstream stream(str);
//     std::string name, model_path, texture_path;
//     stream >> name >> model_path >> texture_path;

//     model_path = route_path + model_path;
//     texture_path = route_path + texture_path;

//     ModelData model_data{model_path, texture_path};
//     model_map.insert({name, model_data});

//     if (texture_map.find(texture_path) != texture_map.end())
//     {
//         return;
//     }

//     auto texture_data = vsg::read_cast<vsg::Data>(texture_path, options);
//     if (!texture_data)
//     {
//         std::cerr << "Failed to read texture file \""
//                   << texture_path << "\"\n";
//     }
//     else
//     {
//         texture_map.insert({texture_path, texture_data});
//     }
// }

void load_model_paths(vsg::ref_ptr<const vsg::Options> options)
{
    std::ifstream file(route_path + "/objects.ref");
    std::string str;

    // unsigned int v1 = std::thread::hardware_concurrency();
    // std::thread** threads = new std::thread*[v1];
    // for (unsigned int i = 0; i < v1; ++i)
    // {
    //     threads[i] = nullptr;
    // }
    // unsigned int v2 = 0;

    while (std::getline(file, str))
    {
        if (str.empty() || str[0] == ';' || str[0] == '[' || str[0] == ':')
        {
            continue;
        }

        // threads[v2++] = new std::thread(handle_str, str, options);
        // if (v2 == v1)
        // {
        //     for (unsigned int i = 0; i < v2; ++i)
        //     {
        //         if (threads[i])
        //         {
        //             threads[i]->join();
        //             delete threads[i];
        //             threads[i] = nullptr;
        //         }
        //     }
        // }

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

    // delete[] threads;
}

void load_model_transformations()
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

void add_models_to_scene_graph(vsg::ref_ptr<vsg::Group> scene_graph,
                               vsg::ref_ptr<const vsg::Options> options)
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
            if (scene_graph->children.size() > 600)
            {
                break;
            }
        }
    }

    model_map.clear();
    texture_map.clear();
}
