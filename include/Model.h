#ifndef ANI_MODEL_H
#define ANI_MODEL_H

#include "Mesh.h"

#include <vsg/io/Path.h>

#include <vector>

class Model
{
public:
    static Model loadDmd(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> options);

    const std::vector<Mesh>& meshes() const { return meshes_; }

private:
    std::vector<Mesh> meshes_;
};

#endif // ANI_MODEL_H
