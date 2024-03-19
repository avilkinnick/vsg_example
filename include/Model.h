#ifndef ANI_MODEL_H
#define ANI_MODEL_H

#include "Mesh.h"

#include <vsg/core/Inherit.h>
#include <vsg/core/Object.h>

#include <vector>

struct Model : public vsg::Inherit<vsg::Object, Model>
{
    std::vector<Mesh> meshes;
};

#endif // ANI_MODEL_H
