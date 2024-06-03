#include "Application.h"

#include "DMD_Reader.h"

#include <iostream>
#include <stdexcept>
#include <chrono>

Application::Application(int* argc, char** argv)
    : arguments(argc, argv)
{
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cerr);
        throw std::runtime_error("Failed to read arguments!");
    }
}

void Application::run()
{
    initialize();
    update();
}

void Application::initialize()
{
    initializeOptions();
    loadShaders();
    initializeWindow();
    initializeCamera();
    initializeSceneGraph();
    createLights();
    initializeCommandGraph();
    initializeViewer();
}

void Application::update()
{
    auto numFrames = arguments.value(-1, "-f");

    auto startTime = vsg::clock::now();
    double numFramesCompleted = 0.0;

    // rendering main loop
    while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
    {
        // pass any events into EventHandlers assigned to the Viewer
        viewer->handleEvents();
        viewer->update();

        auto duration = std::chrono::duration<double, std::chrono::seconds::period>(vsg::clock::now() - startTime).count();
        duration /= 3;
        sunLight->direction.set(cos(duration), -1.0, sin(duration));
        sunLight->direction = vsg::normalize(sunLight->direction);

        viewer->recordAndSubmit();

        viewer->present();

        numFramesCompleted += 1.0;
    }

    auto duration = std::chrono::duration<double, std::chrono::seconds::period>(vsg::clock::now() - startTime).count();
    if (numFramesCompleted > 0.0)
    {
        std::cout << "Average frame rate = " << (numFramesCompleted / duration) << std::endl;
    }
}

void Application::initializeOptions()
{
    options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->add(DMD_Reader::create());
    options->sharedObjects = vsg::SharedObjects::create();
    for (auto& path : options->paths)
    {
        std::cout << path << "\n";
    }
}

void Application::loadShaders()
{
    vsg::Paths searchPaths = {"../"};

    auto phong = vsg::createPhongShaderSet(options);
    if (!phong)
    {
        return;
    }
    // phong->optionalDefines.insert("SHADOWMAP_DEBUG");
    // phong->defaultShaderHints = vsg::ShaderCompileSettings::create();
    // phong->defaultShaderHints->defines.insert("SHADOWMAP_DEBUG");
    phong->variants.clear();

    options->shaderSets["phong"] = phong;
}

void Application::initializeSceneGraph()
{
    const std::string route_path = "../routes/agryz-krugloe_pole";

    sceneGraph = vsg::Group::create();

    loadObjectsRef(route_path);
    loadRouteMap(route_path);
}

void Application::createLights()
{
    auto ambientLight = vsg::AmbientLight::create();
    ambientLight->name = "ambient";
    ambientLight->color.set(1.0, 1.0, 1.0);
    ambientLight->intensity = 0.3;
    sceneGraph->addChild(ambientLight);

    sunLight = vsg::DirectionalLight::create();
    sunLight->name = "sun";
    sunLight->color.set(255.0 / 255.0, 238.0 / 255.0, 80.0 / 255.0);
    sunLight->intensity = 1.0;
    sunLight->direction.set(1.0, 1.0, 1.0);
    sunLight->direction = vsg::normalize(sunLight->direction);
    sunLight->shadowMaps = 1;
    sceneGraph->addChild(sunLight);
}

void Application::loadObjectsRef(const std::string& routePath)
{
    std::set<std::string> invalid_textures;
    bool mipmap = false;
    bool smooth = false;

    std::ifstream file(routePath + "/objects.ref");
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == ':')
        {
            continue;
        }
        else if (line == "[mipmap]")
        {
            mipmap = true;
        }
        else if (line == "[not_mipmap]")
        {
            mipmap = false;
        }
        else if (line == "[smooth]")
        {
            smooth = true;
        }
        else if (line == "[not_smooth]")
        {
            smooth = false;
        }
        else
        {
            std::istringstream stream(line);
            std::string label, model_path, texture_path;
            stream >> label >> model_path >> texture_path;

            model_path = routePath + model_path;
            texture_path = routePath + texture_path;

            ObjectRef object_ref;
            object_ref.label = label;
            object_ref.modelPath = model_path;
            object_ref.texturePath = texture_path;
            object_ref.mipmap = mipmap;
            object_ref.smooth = smooth;
            objectsRef.push_back(object_ref);
        }
    }
}

void Application::loadRouteMap(const std::string& routePath)
{
    std::ifstream file(routePath + "/route1.map");
    std::string line;

    while (std::getline(file, line))
    {
        while (true)
        {
            auto CR_pos = line.find('\r');
            if (CR_pos == std::string::npos)
            {
                break;
            }
            line.erase(CR_pos);
        }

        if (line.empty() || line.back() != ';' || line[0] == ',')
        {
            continue;
        }
        else
        {
            line.pop_back();
            std::replace(line.begin(), line.end(), ',', ' ');

            std::istringstream stream(line);
            std::string label;
            vsg::dvec3 translation, rotation;
            stream >> label >> translation >> rotation;

            rotation.x = vsg::radians(rotation.x);
            rotation.y = vsg::radians(rotation.y);
            rotation.z = vsg::radians(rotation.z);

            ObjectTransformation object_transformation;
            object_transformation.reference = nullptr;
            object_transformation.translation = translation;
            object_transformation.rotation = rotation;

            for (ObjectRef& ref : objectsRef)
            {
                if (ref.label == label)
                {
                    object_transformation.reference = &ref;
                    break;
                }
            }

            if (object_transformation.reference != nullptr)
            {
                objectTransformations.push_back(object_transformation);
            }
        }
    }
}

void Application::initializeWindow()
{
    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->debugLayer = true;
    windowTraits->debugUtils = true;
    windowTraits->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    windowTraits->vulkanVersion = VK_API_VERSION_1_3;
    auto deviceFeatures = windowTraits->deviceFeatures = vsg::DeviceFeatures::create();
    deviceFeatures->get().samplerAnisotropy = VK_TRUE;

    window = vsg::Window::create(windowTraits);
    if (!window)
    {
        throw std::runtime_error("Failed to create window!");
    }
}

void Application::initializeCamera()
{
    const VkExtent2D& extent = window->extent2D();

    constexpr double FOV = 60.0;
    const double aspectRatio = static_cast<double>(extent.width) / static_cast<double>(extent.height);
    constexpr double nearFarRatio = 0.001;

    vsg::dvec3 center = vsg::dvec3(0.0, 0.0, 0.0);
    double radius = 1000.0;

    vsg::dvec3 eye = center + vsg::dvec3(100.0, 100.0, 100.0);
    vsg::dvec3 up = vsg::dvec3(0.0, 0.0, 1.0);

    auto viewport = vsg::ViewportState::create(0, 0, extent.width, extent.height);
    auto perspective = vsg::Perspective::create(60.0, aspectRatio, nearFarRatio * radius, radius * 10.0);
    lookAt = vsg::LookAt::create(eye, center, up);

    camera = vsg::Camera::create(perspective, lookAt, viewport);
}

void Application::initializeCommandGraph()
{
    double maxShadowDistance = arguments.value<double>(100.0, "--sd");
    // double shadowMapBias = arguments.value<double>(0.005, "--sb");
    double shadowMapBias = arguments.value<double>(0.005, "--sb");
    double lambda = arguments.value<double>(0.05, "--lambda");

    auto view = vsg::View::create();
    view->camera = camera;
    view->viewDependentState->maxShadowDistance = maxShadowDistance;
    view->viewDependentState->shadowMapBias = shadowMapBias;
    view->viewDependentState->lambda = lambda;
    view->addChild(sceneGraph);

    auto renderGraph = vsg::RenderGraph::create(window, view);
    commandGraph = vsg::CommandGraph::create(window, renderGraph);
}

void Application::initializeViewer()
{
    for (const ObjectTransformation& transformation : objectTransformations)
    {
        const std::string paths = transformation.reference->modelPath + transformation.reference->texturePath;

        const vsg::dvec3& translation = transformation.translation;
        const vsg::dvec3& rotation = transformation.rotation;

        vsg::dmat4 m1 = vsg::translate(translation);
        vsg::dmat4 m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
        vsg::dmat4 m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
        vsg::dmat4 m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));

        auto model = vsg::read_cast<vsg::StateGroup>(paths, options);
        if (!model)
        {
            continue;
        }

        vsg::ComputeBounds bounds;
        model->accept(bounds);

        vsg::dvec3 center = (bounds.bounds.min + bounds.bounds.max) * 0.5;
        double radius = 200.0;

        auto paged_lod = vsg::PagedLOD::create();
        paged_lod->options = options;
        paged_lod->filename = paths;
        paged_lod->children[0].minimumScreenHeightRatio = 0.2;
        paged_lod->bound = vsg::dsphere(center, radius);

        auto matrix_transform = vsg::MatrixTransform::create();
        matrix_transform->matrix = m1 * m2 * m3 * m4;
        matrix_transform->addChild(paged_lod);

        sceneGraph->addChild(matrix_transform);
    }

    DMD_Reader::models_loaded = true;

    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    auto resource_hints = vsg::ResourceHints::create();
    resource_hints->numDescriptorSets = 4;

    viewer->compile(resource_hints);
}
