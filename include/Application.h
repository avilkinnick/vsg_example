#ifndef APPLICATION_H
#define APPLICATION_H

#include <vsg/all.h>
#include <vsgXchange/all.h>

struct ObjectRef
{
    std::string label;
    std::string model_path;
    std::string texture_path;
    bool mipmap;
    bool smooth;
};

struct ObjectTransformation
{
    ObjectRef* reference;
    vsg::dvec3 translation;
    vsg::dvec3 rotation;
};

class Application
{
public:
    Application(int* argc, char** argv);
    ~Application();

    void run();

private:
    void initialize();
    void main_loop();

    void initialize_options();

    void initialize_scene_graph();

    void load_objects_ref(const std::string& route_path);
    void load_route_map(const std::string& route_path);

    void initialize_window();
    void initialize_camera();
    void initialize_command_graph();
    void initialize_viewer();

private:
    vsg::CommandLine arguments;
    vsg::ref_ptr<vsg::Options> options;

    vsg::ref_ptr<vsg::Group> scene_graph;
    // vsg::ComputeBounds compute_bounds;
    vsg::ref_ptr<vsg::Window> window;
    vsg::ref_ptr<vsg::Camera> camera;
    vsg::ref_ptr<vsg::CommandGraph> command_graph;
    vsg::ref_ptr<vsg::Viewer> viewer;
    // vsg::ref_ptr<vsg::OperationThreads> load_threads;

    std::vector<ObjectRef> objects_ref;

    std::vector<ObjectTransformation> object_transformations;
};

#endif // APPLICATION_H
