#ifndef ANI_MESH_H
#define ANI_MESH_H

#include <vsg/core/Array.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/io/Path.h>

class Mesh
{
public:
    static Mesh loadDmd(const vsg::Path& path);

private:
    vsg::ref_ptr<vsg::vec3Array> vertices_;
    vsg::ref_ptr<vsg::vec3Array> normals_;
    vsg::ref_ptr<vsg::vec3Array> texCoords_;
    vsg::ref_ptr<vsg::ushortArray> indices_;
};

#endif // ANI_MESH_H
