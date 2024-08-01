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
    createShaderSet();
    initializeWindow();
    initializeCamera();
    initializeSceneGraph();
    createLights();
    createView();
    initializeCommandGraph();
    initializeViewer();
}

void Application::update()
{
    auto numFrames = arguments.value(-1, "-f");

    auto startTime = vsg::clock::now();
    double numFramesCompleted = 0.0;

    while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
    {
        viewer->handleEvents();
        viewer->update();

        auto duration = std::chrono::duration<double, std::chrono::seconds::period>(vsg::clock::now() - startTime).count();
        sunLight->direction.set(cos(duration), -1.0, sin(duration));

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
    // options->add(vsgXchange::all::create());
    options->add(DMD_Reader::create());
    DMD_Reader::init();
    options->sharedObjects = vsg::SharedObjects::create();
}

void Application::createShaderSet()
{
    auto shaderHints = vsg::ShaderCompileSettings::create();
    shaderHints->defines.insert("VSG_SHADOWS_PCSS");
    shaderHints->defines.insert("VSG_ALPHA_TEST");

    auto v1 = vsg::TileDatabaseSettings::create();

    auto phong = vsg::createPhongShaderSet(options);
    if (!phong)
    {
        return;
    }

    phong->defaultShaderHints = shaderHints;
    phong->variants.clear();

    options->shaderSets["phong"] = phong;
}

void Application::initializeSceneGraph()
{
    const std::string route_path = "../routes/rostov-kavkazskaya";

    sceneGraph = vsg::Group::create();

    loadObjectsRef(route_path);
    loadRouteMap(route_path);
}

void Application::createLights()
{
    auto shadowSettings = vsg::PercentageCloserSoftShadows::create(1);

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
    sunLight->shadowSettings = shadowSettings;
    sceneGraph->addChild(sunLight);
}

void Application::createView()
{
    double maxShadowDistance = arguments.value<double>(200.0, "--sd");
    double shadowMapBias = arguments.value<double>(0.005, "--sb");
    double lambda = arguments.value<double>(0.05, "--lambda");

    view = vsg::View::create();
    view->camera = camera;
    view->viewDependentState->maxShadowDistance = maxShadowDistance;
    view->viewDependentState->shadowMapBias = shadowMapBias;
    view->viewDependentState->lambda = lambda;
    view->addChild(sceneGraph);
}

void Application::loadObjectsRef(const std::string& routePath)
{
    std::set<std::string> invalidTextures;
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
            std::string label, modelPath, texturePath;
            stream >> label >> modelPath >> texturePath;

            modelPath = routePath + modelPath;
            texturePath = routePath + texturePath;

            ObjectRef objectRef;
            objectRef.label = label;
            objectRef.modelPath = modelPath;
            objectRef.texturePath = texturePath;
            objectRef.mipmap = mipmap;
            objectRef.smooth = smooth;
            objectsRef.push_back(objectRef);
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

            ObjectTransformation objectTransformation;
            objectTransformation.reference = nullptr;
            objectTransformation.translation = translation;
            objectTransformation.rotation = rotation;

            for (ObjectRef& ref : objectsRef)
            {
                if (ref.label == label)
                {
                    objectTransformation.reference = &ref;
                    break;
                }
            }

            if (objectTransformation.reference != nullptr)
            {
                objectTransformations.push_back(objectTransformation);
            }
        }
    }
}

void Application::initializeWindow()
{
    auto windowTraits = vsg::WindowTraits::create();
    // windowTraits->debugLayer = true;
    // windowTraits->debugUtils = true;
    // windowTraits->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    // windowTraits->vulkanVersion = VK_API_VERSION_1_3;

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
    auto renderGraph = vsg::RenderGraph::create(window, view);
    commandGraph = vsg::CommandGraph::create(window, renderGraph);
}

void Application::initializeViewer()
{
    for (const ObjectTransformation& transformation : objectTransformations)
    {
        const vsg::Path paths = transformation.reference->modelPath + " " + transformation.reference->texturePath + " qqqqqq.qqqqqq";

        auto pagedLod = vsg::PagedLOD::create();
        pagedLod->options = options;
        pagedLod->filename = paths;
        pagedLod->children[0].minimumScreenHeightRatio = 0.2;
        pagedLod->bound = vsg::dsphere(vsg::dvec3(0.0, 0.0, 0.0), 100);

        const vsg::dvec3& translation = transformation.translation;
        const vsg::dvec3& rotation = transformation.rotation;

        vsg::dmat4 m1 = vsg::translate(translation);
        vsg::dmat4 m2 = vsg::rotate(-rotation.z, vsg::dvec3(0.0f, 0.0f, 1.0f));
        vsg::dmat4 m3 = vsg::rotate(-rotation.x, vsg::dvec3(1.0f, 0.0f, 0.0f));
        vsg::dmat4 m4 = vsg::rotate(-rotation.y, vsg::dvec3(0.0f, 1.0f, 0.0f));

        auto matrixTransform = vsg::MatrixTransform::create();
        matrixTransform->matrix = m1 * m2 * m3 * m4;
        matrixTransform->addChild(pagedLod);

        sceneGraph->addChild(matrixTransform);
    }

    objectTransformations.clear();
    objectsRef.clear();

    viewer = vsg::Viewer::create();
    viewer->addWindow(window);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    viewer->compile();
}
