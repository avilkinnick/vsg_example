#include "DMD_Reader.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>

struct Model
{
    std::string model_path;
    std::string texture_path;
    vsg::vec3 translation;
    vsg::vec3 rotation;
};

int main(int argc, char* argv[])
{
    try
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

        auto scene_graph = vsg::Group::create();

        std::string route_path = "../routes/konotop-suchinichi";
        std::map<std::string, vsg::ref_ptr<vsg::Data>> textures;
        std::map<std::string, Model> models;
        std::vector<Model> models2;
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

                Model model;
                model.model_path = route_path + model_path;
                model.texture_path = route_path + texture_path;
                models.insert({name, model});

                if (textures.find(texture_path) == textures.end())
                {
                    auto texture_data = vsg::read_cast<vsg::Data>(model.texture_path, options);
                    if (!texture_data)
                    {
                        std::cerr << "Failed to read texture file \"" << model.texture_path << "\"\n";
                    }
                    else
                    {
                        textures.insert({ model.texture_path, texture_data });
                    }
                }
            }
        }

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
                double tx, ty, tz, rx, ry, rz;
                stream >> name >> tx >> ty >> tz >> rx >> ry >> rz;
                rx = vsg::radians(rx);
                ry = vsg::radians(ry);
                rz = vsg::radians(rz);

                if (models.find(name) != models.end())
                {
                    auto& model = models.at(name);
                    models2.emplace_back(Model{model.model_path, model.texture_path, vsg::vec3(tx, ty, tz), vsg::vec3(rx, ry, rz)});
                }

            }
        }

        models.clear();

        for (auto& model_data : models2)
        {
            vsg::ref_ptr<vsg::Data> texture_data;
            if (textures.find(model_data.texture_path) != textures.end())
            {
                texture_data = textures.at(model_data.texture_path);
            }

            auto model = DMD_Reader().read(model_data.model_path, texture_data, options);

            if (model)
            {
                const auto& rotation = model_data.rotation;
                auto m1 = vsg::translate(model_data.translation);
                auto m2 = vsg::rotate(-rotation.z, vsg::vec3(0.0f, 0.0f, 1.0f));
                auto m3 = vsg::rotate(-rotation.x, vsg::vec3(1.0f, 0.0f, 0.0f));
                auto m4 = vsg::rotate(-rotation.y, vsg::vec3(0.0f, 1.0f, 0.0f));
                model->matrix = m1 * m2 * m3 * m4;

                scene_graph->addChild(model);
                // scene_graph->addChild(vsg::LOD::Child{50.0, model});
                if (scene_graph->children.size() > 600)
                    break;
            }
        }

        models2.clear();
        textures.clear();

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
        double window_width = static_cast<double>(window->extent2D().width);
        double window_height = static_cast<double>(window->extent2D().height);
        double aspect_ratio = window_width / window_height;
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
        // trackball->moveForwardKey = vsg::KEY_w;
        // trackball->moveBackwardKey = vsg::KEY_s;
        // trackball->moveLeftKey = vsg::KEY_a;
        // trackball->moveRightKey = vsg::KEY_d;
        // trackball->turnLeftKey = vsg::KEY_Leftcurlybracket;
        // trackball->turnRightKey = vsg::KEY_Leftcurlybracket;
        // trackball->pitchUpKey = vsg::KEY_Leftcurlybracket;
        // trackball->pitchDownKey = vsg::KEY_Leftcurlybracket;
        // trackball->rollLeftKey = vsg::KEY_Leftcurlybracket;
        // trackball->rollRightKey = vsg::KEY_Leftcurlybracket;
        // trackball->moveUpKey = vsg::KEY_Leftcurlybracket;
        // trackball->moveDownKey = vsg::KEY_Leftcurlybracket;

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

