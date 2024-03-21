#ifndef ANI_VSG_INSTANCE_H
#define ANI_VSG_INSTANCE_H

#include <vsg/all.h>
#include <vsgXchange/all.h>

class vsg_Instance
{
public:
    vsg_Instance(int* argc, char** argv);

    void run();

public:
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::WindowTraits> windowTraits;
    vsg::CommandLine arguments;
    vsg::ref_ptr<vsg::Group> sceneGraph;
    vsg::ref_ptr<vsg::Viewer> viewer;
    vsg::ref_ptr<vsg::Window> window;
    vsg::ref_ptr<vsg::Camera> camera;
    vsg::ref_ptr<vsg::CommandGraph> commandGraph;
};

#endif // ANI_VSG_INSTANCE_H
