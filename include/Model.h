#ifndef ANI_MODEL_H
#define ANI_MODEL_H

#include "Mesh.h"

#include <vsg/core/Inherit.h>

#include <vector>

struct Model : public vsg::Inherit<vsg::Object, Model>
{
public:
    std::vector<Mesh> meshes;
};

#endif // ANI_MODEL_H
