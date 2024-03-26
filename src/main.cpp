#include "DMD_Reader.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>

struct Model
{
    std::string model_path;
    std::string texture_path;
    vsg::t_vec3<double> translation;
    vsg::t_vec3<double> rotation;
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

                if (textures.find(texture_path) == textures.end())
                {
                    auto texture_data = vsg::read_cast<vsg::Data>(texture_path, options);
                    if (!texture_data)
                    {
                        std::cerr << "Failed to read texture file \"" << texture_path << "\"\n";
                    }
                    else
                    {
                        auto texture = vsg::Image::create(texture_data);

                        textures.insert({ texture_path, texture_data });

                        Model model;
                        model.model_path = model_path;
                        model.texture_path = texture_path;
                        models.insert({name, model});
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
                rx *= vsg::PIf / 180.0f;
                ry *= vsg::PIf / 180.0f;
                rz *= vsg::PIf / 180.0f;

                if (models.find(name) != models.end())
                {
                    models.at(name).translation.set(tx, ty, tz);
                    models.at(name).rotation.set(rx, ry, rz);
                }
            }
        }

        for (auto& model_pair : models)
        {
            auto& model_data = model_pair.second;
            auto model = DMD_Reader().read(model_data.model_path, textures.at(model_data.texture_path), options);
            const auto& rotation = model_data.rotation;
            auto m1 = vsg::translate(model_data.translation);
            auto m2 = vsg::rotate(-rotation.z, vsg::t_vec3<double>(0.0, 0.0, 1.0));
            auto m3 = vsg::rotate(-rotation.x, vsg::t_vec3<double>(1.0, 0.0, 0.0));
            auto m4 = vsg::rotate(-rotation.y, vsg::t_vec3<double>(0.0, 1.0, 0.0));
            // model->matrix = m4 * m3 * m2 * m1;

            scene_graph->addChild(model);
        }

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
        auto window_width = static_cast<double>(window->extent2D().width);
        auto window_height = static_cast<double>(window->extent2D().height);
        auto viewport = vsg::ViewportState::create(0, 0, window_width, window_height);
        auto perspective = vsg::Perspective::create(60.0, window_width / window_height, near_far_ratio * radius, radius * 10.0);
        auto look_at = vsg::LookAt::create(centre + vsg::dvec3(100.0, 100.0, 100.0), centre + vsg::dvec3(10.0, 10.0, 10.0), vsg::dvec3(0.0, 0.0, 1.0));
        auto camera = vsg::Camera::create(perspective, look_at, viewport);

        auto command_graph = vsg::createCommandGraphForView(window, camera, scene_graph);

        auto viewer = vsg::Viewer::create();
        viewer->addWindow(window);
        viewer->assignRecordAndSubmitTaskAndPresentation({command_graph});
        viewer->compile();
        viewer->addEventHandler(vsg::CloseHandler::create(viewer));
        viewer->addEventHandler(vsg::Trackball::create(camera));

        while (viewer->advanceToNextFrame())
        {
            viewer->handleEvents();

            auto keyboard = vsg::Keyboard::create();

            viewer->update();
            viewer->recordAndSubmit();
            viewer->present();
        }
    }
    catch (vsg::Exception error)
    {
        std::cerr << error.message << '\n';
    }

    return 0;
}

