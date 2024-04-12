#ifndef DMD_READER_H
#define DMD_READER_H

#include <vsg/all.h>

#include <string>

struct ModelData : public vsg::Inherit<vsg::Object, ModelData>
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::vec3Array> normals;
    vsg::ref_ptr<vsg::vec3Array> tex_coords;
    vsg::ref_ptr<vsg::vec4Array> colors;
    vsg::ref_ptr<vsg::ushortArray> indices;
};

class DMD_Reader : public vsg::Inherit<vsg::ReaderWriter, DMD_Reader>
{
public:
    vsg::ref_ptr<vsg::Object> read(
        const vsg::Path&,
        vsg::ref_ptr<const vsg::Options> = {}
    ) const override;

private:
    void remove_CR_symbols(std::string& str) const;
};

#endif // DMD_READER_H
