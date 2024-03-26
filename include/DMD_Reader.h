#ifndef DMD_READER_H
#define DMD_READER_H

#include "Mesh.h"

#include <vsg/core/Array.h>
#include <vsg/core/ref_ptr.h>
#include <vsg/io/Options.h>
#include <vsg/io/Path.h>
#include <vsg/io/txt.h>
#include <vsg/nodes/StateGroup.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

class DMD_Reader : public vsg::txt
{
public:
    DMD_Reader();

    vsg::ref_ptr<vsg::MatrixTransform> read(
        const vsg::Path& model_path,
        vsg::ref_ptr<vsg::Data> texture_data,
        vsg::ref_ptr<const vsg::Options> options
    );

private:
    vsg::ref_ptr<vsg::MatrixTransform> m_model;
    std::vector<Mesh> m_meshes;
    Mesh* m_current_mesh = nullptr;

    vsg::ref_ptr<vsg::vec3Array>   m_temp_vertices;
    vsg::ref_ptr<vsg::ushortArray> m_temp_indices;

    std::size_t m_vertices_count = 0;
    std::size_t m_faces_count = 0;
    std::size_t m_indices_count = 0;
    std::size_t m_temp_vertices_count = 0;

    vsg::ref_ptr<vsg::vec3Array>   m_model_vertices;
    vsg::ref_ptr<vsg::vec3Array>   m_model_normals;
    vsg::ref_ptr<vsg::vec3Array>   m_model_tex_coords;
    vsg::ref_ptr<vsg::vec4Array>   m_model_colors;
    vsg::ref_ptr<vsg::ushortArray> m_model_indices;

private:
    void remove_CR_symbols(std::string& str);

    void add_mesh();
    void reset_mesh_arrays();
    void read_numverts_and_numfaces(std::stringstream& stream);
    void init_temp_arrays();
    void read_mesh_vertices(std::stringstream& stream);
    void read_mesh_faces(std::stringstream& stream);
    void read_numtverts_and_numtvfaces(std::stringstream& stream);
    void init_mesh_arrays();
    void read_texture_vertices(std::stringstream& stream);
    void set_default_color();
    void read_and_proceed_texture_faces(std::stringstream& stream);
    void calculate_vertex_normals();

    void combine_meshes_array();
};

#endif // DMD_READER_H
