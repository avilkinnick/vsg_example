#ifndef ANI_MESH_H
#define ANI_MESH_H

#include <vsg/core/Array.h>
#include <vsg/core/ref_ptr.h>

struct Mesh
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::vec3Array> normals;
    vsg::ref_ptr<vsg::vec3Array> tex_coords;
    vsg::ref_ptr<vsg::vec4Array> colors;
    vsg::ref_ptr<vsg::ushortArray> indices;
};

#endif // ANI_MESH_H
