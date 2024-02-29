#include <vsg/all.h>

#include <iostream>

int main(int argc, char* argv[])
{
    auto windowTraits = vsg::WindowTraits::create();

    vsg::ref_ptr<vsg::ShaderSet> shaderSet = vsg::createPhongShaderSet(vsg::Options::create());

    if (!shaderSet)
    {
        std::cout << "Could not create shaders." << std::endl;
        return 1;
    }

    vsg::dvec3 position     { 0.0, 0.0, 0.0 };
    vsg::dvec3 delta_column { 2.0, 0.0, 0.0 };
    vsg::dvec3 delta_row    { 0.0, 2.0, 0.0 };

    auto scenegraph = vsg::Group::create();

    auto graphicsPipelineConfig = vsg::GraphicsPipelineConfigurator::create(shaderSet);

    // set up passing of material
    auto mat = vsg::PhongMaterialValue::create();
    mat->value().diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat->value().specular.set(1.0f, 0.0f, 0.0f, 1.0f); // red specular highlight

    graphicsPipelineConfig->assignDescriptor("material", mat);

    // set up vertex and index arrays
    auto vertices = vsg::vec3Array::create(
        {{-0.5f, -0.5f, -0.5f},
         {-0.5f, -0.5f,  0.5f},
         {-0.5f,  0.5f, -0.5f},
         {-0.5f,  0.5f,  0.5f},
         { 0.5f, -0.5f, -0.5f},
         { 0.5f, -0.5f,  0.5f},
         { 0.5f,  0.5f, -0.5f},
         { 0.5f,  0.5f,  0.5f}});

    auto normals = vsg::vec3Array::create(
        {{-1.0f, -1.0f, -1.0f},
         {-1.0f, -1.0f,  1.0f},
         {-1.0f,  1.0f, -1.0f},
         {-1.0f,  1.0f,  1.0f},
         { 1.0f, -1.0f, -1.0f},
         { 1.0f, -1.0f,  1.0f},
         { 1.0f,  1.0f, -1.0f},
         { 1.0f,  1.0f,  1.0f}});

    // auto texcoords = vsg::vec2Array::create(
    //     {{0.0f, 0.0f},
    //      {1.0f, 0.0f},
    //      {1.0f, 1.0f},
    //      {0.0f, 1.0f},
    //      {0.0f, 0.0f},
    //      {1.0f, 0.0f},
    //      {1.0f, 1.0f},
    //      {0.0f, 1.0f}});

    auto colors = vsg::vec4Array::create(
        {{0.0f, 0.0f, 0.0f, 1.0f},
         {0.0f, 0.0f, 1.0f, 1.0f},
         {0.0f, 1.0f, 0.0f, 1.0f},
         {0.0f, 1.0f, 1.0f, 1.0f},
         {1.0f, 0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 1.0f, 1.0f},
         {1.0f, 1.0f, 0.0f, 1.0f},
         {1.0f, 1.0f, 1.0f, 1.0f}});

    auto indices = vsg::ushortArray::create({
        // Top
        1, 5, 3,
        3, 5, 7,

        // Bottom
        0, 2, 4,
        4, 2, 6,

        // Left
        0, 1, 2,
        2, 1, 3,

        // Front
        0, 4, 1,
        1, 4, 5,

        // Back
        2, 3, 6,
        6, 3, 7,

        // Right
        4, 6, 5,
        5, 6, 7
    });

    vsg::DataList vertexArrays;

    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, vertices);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX, normals);
    // graphicsPipelineConfig->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, texcoords);
    graphicsPipelineConfig->assignArray(vertexArrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX, colors);

    auto drawCommands = vsg::Commands::create();
    drawCommands->addChild(vsg::BindVertexBuffers::create(graphicsPipelineConfig->baseAttributeBinding, vertexArrays));
    drawCommands->addChild(vsg::BindIndexBuffer::create(indices));
    drawCommands->addChild(vsg::DrawIndexed::create(36, 1, 0, 0, 0));

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
    return 0;
}

