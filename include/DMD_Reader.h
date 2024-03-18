#ifndef DMD_READER_H
#define DMD_READER_H

#include "Model.h"

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <sstream>

class DMD_Reader : public vsg::txt
{
public:
    DMD_Reader();

    vsg::ref_ptr<Model> read(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> options);

private:
    vsg::ref_ptr<Model> model_;
    Mesh* currentMesh_ = nullptr;

    vsg::ref_ptr<vsg::vec3Array> tempVertices_;
    vsg::ref_ptr<vsg::ushortArray> tempIndices_;

    std::size_t verticesCount_;
    std::size_t facesCount_;
    std::size_t tempVerticesCount_;

private:
    void remove_CR_symbols(std::string& str);

    void add_mesh();
    void read_numverts_and_numfaces(std::stringstream& stream);
    void init_temp_arrays();
    void read_mesh_vertices(std::stringstream& stream);
    void read_mesh_faces(std::stringstream& stream);
    void read_numtverts_and_numtvfaces(std::stringstream& stream);
    void init_mesh_arrays();
    void read_texture_vertices(std::stringstream& stream);
    void set_default_color();
};

#endif // DMD_READER_H
