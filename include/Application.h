#ifndef APPLICATION_H
#define APPLICATION_H

#include <vsg/all.h>
#include <vsgXchange/all.h>

//------------------------------
struct ModelData
{
    std::string model_path;
    std::string texture_path;
    vsg::dvec3 translation;
    vsg::dvec3 rotation;
};
//--------------------------------

class Application
{
public:
    Application(int* argc, char** argv);

    void run();

private:
    void initialize();
    void main_loop();

    void initialize_options();

    void initialize_scene_graph();
    void load_model_paths();
    void load_model_transformations();
    void add_models_to_scene_graph();

    void initialize_window();
    void initialize_camera();
    void initialize_command_graph();
    void initialize_viewer();

private:
    vsg::CommandLine arguments;
    vsg::ref_ptr<vsg::Options> options;

    vsg::ref_ptr<vsg::Group> scene_graph;
    vsg::ComputeBounds compute_bounds;
    vsg::ref_ptr<vsg::Window> window;
    vsg::ref_ptr<vsg::Camera> camera;
    vsg::ref_ptr<vsg::CommandGraph> command_graph;
    vsg::ref_ptr<vsg::Viewer> viewer;

    //-------------------------------
    const std::string route_path = "../routes/konotop-suchinichi";
    std::map<std::string, ModelData> model_map;
    std::map<std::string, vsg::ref_ptr<vsg::Data>> texture_map;
    //-------------------------------
};

#endif // APPLICATION_H
