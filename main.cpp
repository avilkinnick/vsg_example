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
    return std::abs(num) < 0.001;
}

template <typename T>
struct vec3
{
    T x;
    T y;
    T z;
};

using vec3f = vec3<float>;
using vec3u = vec3<unsigned int>;

struct Mesh
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::vec3Array> normals;
    vsg::ref_ptr<vsg::ushortArray> indices;

    vsg::ref_ptr<vsg::vec3Array> texcoords;
    vsg::ref_ptr<vsg::ushortArray> texindices;
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
                current_mesh->vertices = vsg::vec3Array::create(numverts);
                current_mesh->normals = vsg::vec3Array::create(numverts);
                for (std::size_t i = 0; i < numverts; ++i)
                {
                    auto& vertex = current_mesh->vertices->at(i);
                    input_file >> vertex.x >> vertex.y >> vertex.z;
                }
            }
            else if (input_string == "Mesh faces:")
            {
                current_mesh->indices = vsg::ushortArray::create(numfaces * 3);
                for (std::size_t i = 0; i < numfaces * 3; ++i)
                {
                    input_file >> current_mesh->indices->at(i);
                }
            }
            else if (input_string == "numtverts numtvfaces")
            {
                input_file >> numtverts >> numtvfaces;
            }
            else if (input_string == "Texture vertices:")
            {
                current_mesh->texcoords = vsg::vec3Array::create(numtverts);
                for (std::size_t i = 0; i < numtverts; ++i)
                {
                    auto& texcoord = current_mesh->texcoords->at(i);
                    input_file >> texcoord.x >> texcoord.y >> texcoord.z;
                }
            }
            else if (input_string == "Texture faces:")
            {
                current_mesh->texindices = vsg::ushortArray::create(numtvfaces * 3);
                for (std::size_t i = 0; i < numtvfaces * 3; ++i)
                {
                    input_file >> current_mesh->texindices->at(i);
                }
            }
        }

        for (std::size_t i = 0; i < numfaces; ++i)
        {
            auto& mesh = *current_mesh;
            auto& vertices = mesh.vertices;
            auto& indices = mesh.indices;

            auto index1 = indices->at(i * 3);
            auto index2 = indices->at(i * 3 + 1);
            auto index3 = indices->at(i * 3 + 2);

            auto& v1 = vertices->at(index1 - 1);
            auto& v2 = vertices->at(index2 - 1);
            auto& v3 = vertices->at(index3 - 1);

            vec3f a { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
            vec3f b { v3.x - v1.x, v3.y - v1.y, v3.z - v1.z };

            auto x = a.y * b.z - a.z * b.y;
            auto y = a.z * b.x - a.x * b.z;
            auto z = a.x * b.y - a.y * b.x;

            auto& normal1 = mesh.normals->at(index1 - 1);
            auto& normal2 = mesh.normals->at(index2 - 1);
            auto& normal3 = mesh.normals->at(index3 - 1);

            normal1.x += x;
            normal1.y += y;
            normal1.z += z;
            normal2.x += x;
            normal2.y += y;
            normal2.z += z;
            normal3.x += x;
            normal3.y += y;
            normal3.z += z;
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

    vsg::Path texture_path("../textures/vokzal_tuapse.bmp");
    if (texture_path)
    {
        auto texture_data = vsg::read_cast<vsg::Data>(texture_path, options);
        if (!texture_data)
        {
            std::cout << "Failed to read texture file: " << texture_path << '\n';
            return EXIT_FAILURE;
        }

        //----------------------------------------------------------------------
        // Uncomment to see texture
        //----------------------------------------------------------------------
        // graphicsPipelineConfig->assignTexture("diffuseMap", texture_data);
    }

    // set up passing of material
    auto mat = vsg::PhongMaterialValue::create();
    mat->value().diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat->value().specular.set(1.0f, 0.0f, 0.0f, 1.0f); // red specular highlight

    graphicsPipelineConfig->assignDescriptor("material", mat);

    auto& mesh = meshes[0];

    auto colors = vsg::vec4Array::create(mesh.vertices->size());
    for (std::size_t i = 0; i < mesh.vertices->size(); ++i)
    {
        colors->at(i).set(1.0f, 1.0f, 1.0f, 1.0f);
    }

    vsg::DataList vertexArrays;

    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, mesh.vertices);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, mesh.normals);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, mesh.texcoords);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, colors);

    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(graphicsPipelineConfig->baseAttributeBinding, vertexArrays));
    drawCommands->addChild(vsg::BindIndexBuffer::create(mesh.indices));
    drawCommands->addChild(vsg::DrawIndexed::create(mesh.indices->size(), 1, 0, 0, 0));

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
    double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;

    std::cout << "centre = " << centre << std::endl;
    std::cout << "radius = " << radius << std::endl;

    // camera related details
    double nearFarRatio = 0.001;
    auto viewport = vsg::ViewportState::create(0, 0, window->extent2D().width, window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(10.0, 10.0, 10.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
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

