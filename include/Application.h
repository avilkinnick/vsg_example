#ifndef APPLICATION_H
#define APPLICATION_H

#include <vsg/all.h>
#include <vsgXchange/all.h>

struct ObjectRef
{
    std::string label;
    std::string modelPath;
    std::string texturePath;
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

    void run();

private:
    void initialize();
    void update();

    void initializeOptions();

    void loadShaders();

    void initializeSceneGraph();

    void createLights();

    void loadObjectsRef(const std::string& routePath);
    void loadRouteMap(const std::string& routePath);

    void initializeWindow();
    void initializeCamera();
    void initializeCommandGraph();
    void initializeViewer();

private:
    vsg::CommandLine arguments;
    vsg::ref_ptr<vsg::Options> options;

    vsg::ref_ptr<vsg::Group> sceneGraph;
    vsg::ref_ptr<vsg::Window> window;
    vsg::ref_ptr<vsg::Camera> camera;
    vsg::ref_ptr<vsg::CommandGraph> commandGraph;
    vsg::ref_ptr<vsg::Viewer> viewer;

    vsg::ref_ptr<vsg::LookAt> lookAt;
    vsg::ref_ptr<vsg::SpotLight> spotLight;
    vsg::ref_ptr<vsg::CullGroup> cullGroup;
    vsg::ref_ptr<vsg::DirectionalLight> sunLight;

    std::vector<ObjectRef> objectsRef;
    std::vector<ObjectTransformation> objectTransformations;
};

#endif // APPLICATION_H
