#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
bool is_zero(T num)
{
    return std::abs(num) < 0.000001;
}

template <typename T>
struct vec3
{
    T x;
    T y;
    T z;
};

using vec3ld = vec3<long double>;
using vec3u = vec3<unsigned int>;

struct Mesh
{
    std::vector<vec3ld> vertices;
    std::vector<vec3ld> normals;
    std::vector<vec3ld> texcoords;
    std::vector<vec3u> faces;
    std::vector<vec3u> texfaces;
};

int main(int argc, char* argv[])
{
    auto options = vsg::Options::create();
    options->add(vsgXchange::all::create());

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = ".dmd viewer";

    vsg::CommandLine arguments(&argc, argv);
    if (arguments.errors())
    {
        return arguments.writeErrorMessages(std::cerr);
    }

    auto scene = vsg::Group::create();
    std::vector<Mesh> meshes;

    for (int i = 1; i < argc; ++i)
    {
        vsg::Path filename = arguments[i];
        std::ifstream input_file(filename.string());

        if (!input_file)
        {
            std::cerr << "Failed to open \"" << filename << "\" for reading!\n";
            return EXIT_FAILURE;
        }

        Mesh* current_mesh = nullptr;
        int numverts = 0;
        int numfaces = 0;
        int numtverts = 0;
        int numtvfaces = 0;

        std::string input_string;
        while (std::getline(input_file, input_string))
        {
            // Remove CR symbol at the end of string
            input_string = input_string.substr(0, input_string.length() - 1);

            if (input_string == "New object")
            {
                current_mesh = &meshes.emplace_back(Mesh());
            }
            else if (input_string == "numverts numfaces")
            {
                input_file >> numverts >> numfaces;
            }
            else if (input_string == "Mesh vertices:")
            {
                for (int i = 0; i < numverts; ++i)
                {
                    vec3ld vertex;
                    input_file >> vertex.x >> vertex.y >> vertex.z;

                    // auto length = std::sqrt(vertex.x * vertex.x + vertex.y * vertex.y + vertex.z * vertex.z);

                    // if (!is_zero(length))
                    // {
                    //     vertex.x /= length;
                    //     vertex.y /= length;
                    //     vertex.z /= length;
                    // }

                    (*current_mesh).vertices.emplace_back(vertex);
                }
            }
            else if (input_string == "Mesh faces:")
            {
                for (int i = 0; i < numfaces; ++i)
                {
                    vec3u face;
                    input_file >> face.x >> face.y >> face.z;
                    (*current_mesh).faces.emplace_back(face);
                }
            }
            else if (input_string == "numtverts numtvfaces")
            {
                input_file >> numtverts >> numtvfaces;
            }
            else if (input_string == "Texture vertices:")
            {
                for (int i = 0; i < numtverts; ++i)
                {
                    vec3ld texcoord;
                    input_file >> texcoord.x >> texcoord.y >> texcoord.z;
                    (*current_mesh).texcoords.emplace_back(texcoord);
                }
            }
            else if (input_string == "Texture faces:")
            {
                for (int i = 0; i < numtvfaces; ++i)
                {
                    vec3u texface;
                    input_file >> texface.x;
                    input_file >> texface.y;
                    input_file >> texface.z;
                    (*current_mesh).texfaces.emplace_back(texface);
                }
            }
        }

        for (int i = 0; i < numfaces; ++i)
        {
            auto& face = (*current_mesh).faces[i];
            auto& v1 = (*current_mesh).vertices[face.x];
            auto& v2 = (*current_mesh).vertices[face.y];
            auto& v3 = (*current_mesh).vertices[face.z];
            vec3ld a { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
            vec3ld b { v3.x - v1.x, v3.y - v1.y, v3.z - v1.z };
            auto x = a.y * b.z - a.z * b.y;
            auto y = a.z * b.x - a.x * b.z;
            auto z = a.x * b.y - a.y * b.x;
            auto length = std::sqrt(x * x + y * y + z * z);

            if (!is_zero(length))
            {
                x /= length;
                y /= length;
                z /= length;
            }

            vec3ld normal { x, y, z };
            (*current_mesh).normals.emplace_back(normal);
        }

        std::cout << '\n';
    }

    vsg::ref_ptr<vsg::ShaderSet> shaderSet = vsg::createPhongShaderSet(vsg::Options::create());

    if (!shaderSet)
    {
        std::cout << "Could not create shaders." << std::endl;
        return 1;
    }

    auto scenegraph = vsg::Group::create();

    auto graphicsPipelineConfig = vsg::GraphicsPipelineConfigurator::create(shaderSet);

    // set up passing of material
    auto mat = vsg::PhongMaterialValue::create();
    mat->value().diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat->value().specular.set(1.0f, 0.0f, 0.0f, 1.0f); // red specular highlight

    graphicsPipelineConfig->assignDescriptor("material", mat);

    auto& mesh = meshes[0];
    auto vertices_count = mesh.vertices.size();
    auto normals_count = mesh.normals.size();
    auto faces_count = mesh.faces.size();

    auto vertices = vsg::vec3Array::create(vertices_count);
    std::cout << "Vertices (" << vertices_count << "):\n";
    for (std::size_t i = 0; i < vertices_count; ++i)
    {
        auto& vertex = mesh.vertices.at(i);
        vertices->at(i).set(vertex.x, vertex.y, vertex.z);
        std::cout << vertices->at(i).x << ' ' << vertices->at(i).y << ' ' << vertices->at(i).z << '\n';
    }
    std::cout << '\n';

    auto normals = vsg::vec3Array::create(normals_count);
    std::cout << "Normals (" << normals_count << "):\n";
    for (std::size_t i = 0; i < normals_count; ++i)
    {
        auto& normal = mesh.normals.at(i);
        normals->at(i).set(normal.x, normal.y, normal.z);
        std::cout << normals->at(i).x << ' ' << normals->at(i).y << ' ' << normals->at(i).z << '\n';
    }
    std::cout << '\n';

    auto indices = vsg::ushortArray::create(faces_count * 3);
    std::cout << "Indices (" << faces_count * 3 << "):\n";
    for (std::size_t i = 0; i < faces_count; ++i)
    {
        auto& face = mesh.faces.at(i);
        indices->at(i * 3) = face.x;
        indices->at(i * 3 + 1) = face.y;
        indices->at(i * 3 + 2) = face.z;
        std::cout << indices->at(i * 3) << ' ' << indices->at(i * 3 + 1) << ' ' << indices->at(i * 3 + 2) << '\n';
    }
    std::cout << '\n';

    auto colors = vsg::vec4Array::create(vertices_count);
    for (std::size_t i = 0; i < vertices_count; ++i)
    {
        colors->at(i).set(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // set up vertex and index arrays
    // auto vertices = vsg::vec3Array::create(
    //     {{-0.5f, -0.5f, -0.5f},
    //      {-0.5f, -0.5f,  0.5f},
    //      {-0.5f,  0.5f, -0.5f},
    //      {-0.5f,  0.5f,  0.5f},
    //      { 0.5f, -0.5f, -0.5f},
    //      { 0.5f, -0.5f,  0.5f},
    //      { 0.5f,  0.5f, -0.5f},
    //      { 0.5f,  0.5f,  0.5f}});

    // auto normals = vsg::vec3Array::create(
    //     {{-1.0f, -1.0f, -1.0f},
    //      {-1.0f, -1.0f,  1.0f},
    //      {-1.0f,  1.0f, -1.0f},
    //      {-1.0f,  1.0f,  1.0f},
    //      { 1.0f, -1.0f, -1.0f},
    //      { 1.0f, -1.0f,  1.0f},
    //      { 1.0f,  1.0f, -1.0f},
    //      { 1.0f,  1.0f,  1.0f}});

    // auto texcoords = vsg::vec2Array::create(
    //     {{0.0f, 0.0f},
    //      {1.0f, 0.0f},
    //      {1.0f, 1.0f},
    //      {0.0f, 1.0f},
    //      {0.0f, 0.0f},
    //      {1.0f, 0.0f},
    //      {1.0f, 1.0f},
    //      {0.0f, 1.0f}});

    // auto colors = vsg::vec4Array::create(
    //     {{1.0f, 1.0f, 1.0f, 1.0f},
    //      {0.0f, 0.0f, 1.0f, 1.0f},
    //      {0.0f, 1.0f, 0.0f, 1.0f},
    //      {0.0f, 1.0f, 1.0f, 1.0f},
    //      {1.0f, 0.0f, 0.0f, 1.0f},
    //      {1.0f, 0.0f, 1.0f, 1.0f},
    //      {1.0f, 1.0f, 0.0f, 1.0f},
    //      {0.0f, 1.0f, 1.0f, 1.0f}});

    // auto indices = vsg::ushortArray::create({
    //     // Top
    //     1, 5, 3,
    //     3, 5, 7,

    //     // Bottom
    //     0, 2, 4,
    //     4, 2, 6,

    //     // Left
    //     0, 1, 2,
    //     2, 1, 3,

    //     // Front
    //     0, 4, 1,
    //     1, 4, 5,

    //     // Back
    //     2, 3, 6,
    //     6, 3, 7,

    //     // Right
    //     4, 6, 5,
    //     5, 6, 7
    // });

    vsg::DataList vertexArrays;

    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, vertices);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, normals);
    // graphicsPipelineConfig->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, texcoords);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, colors);

    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(graphicsPipelineConfig->baseAttributeBinding, vertexArrays));
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(faces_count * 3, 1, 0, 0, 0));

    graphicsPipelineConfig->init();

    // create StateGroup as the root of the scene/command graph to hold the GraphicsPipeline, and binding of Descriptors to decorate the whole graph
    auto stateGroup = vsg::StateGroup::create();

    graphicsPipelineConfig->copyTo(stateGroup);

    // add drawCommands to StateGroup
    stateGroup->addChild(drawCommands);
    scenegraph->addChild(stateGroup);

    // create the viewer and assign window(s) to it
    auto viewer = vsg::Viewer::create();

    auto window = vsg::Window::create(windowTraits);
    if (!window)
    {
        std::cout << "Could not create window." << std::endl;
        return 1;
    }

    viewer->addWindow(window);

    vsg::ComputeBounds computeBounds;
    scenegraph->accept(computeBounds);
    vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6 * 3.0;

    std::cout << "centre = " << centre << std::endl;
    std::cout << "radius = " << radius << std::endl;

    // camera related details
    double nearFarRatio = 0.001;
    auto viewport = vsg::ViewportState::create(0, 0, window->extent2D().width, window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(2.0, 2.0, 2.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, viewport);

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scenegraph);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    // compile the Vulkan objects
    viewer->compile();

    // assign a CloseHandler to the Viewer to respond to pressing Escape or the window close button
    viewer->addEventHandlers({vsg::CloseHandler::create(viewer)});

    viewer->addEventHandler(vsg::Trackball::create(camera));

    // main frame loop
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }

    // clean up done automatically thanks to ref_ptr<>
    return EXIT_SUCCESS;
}

