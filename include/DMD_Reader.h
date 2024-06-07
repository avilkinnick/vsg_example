#ifndef DMD_READER_H
#define DMD_READER_H

#include <vsg/all.h>

#include <string>

struct ModelData : public vsg::Inherit<vsg::Object, ModelData>
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::vec3Array> normals;
    vsg::ref_ptr<vsg::vec2Array> tex_coords;
    vsg::ref_ptr<vsg::vec4Array> colors;
    vsg::ref_ptr<vsg::ushortArray> indices;
};

class DMD_Reader : public vsg::Inherit<vsg::ReaderWriter, DMD_Reader>
{
public:
    vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options = {}) const override;

private:
    vsg::ref_ptr<ModelData> load_model(const vsg::Path& model_file) const;
    void remove_carriage_return_symbols(std::string& str) const;

    static std::map<vsg::Path, vsg::ref_ptr<vsg::StateGroup>> state_groups;
};

#endif // DMD_READER_H
