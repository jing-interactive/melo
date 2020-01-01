#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ObjLoader.h"
#include "cinder/FileWatcher.h"

// vnm
#include "AssetManager.h"
#include "MiniConfig.h"
#include "FontHelper.h"
#include "GLHelper.h"

// melo
#include "melo.h"
#include "NodeExt.h"
#include "FirstPersonCamera.h"

// imgui
#include "MiniConfigImgui.h"
#include "CinderGuizmo.h"
#include "DearLogger.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct MeloViewer : public App
{
    CameraPersp2 mMayaCam;
    CameraUi mMayaCamUi;
    FirstPersonCamera mFpsCam;
    CameraPersp2* mCurrentCam = nullptr;
    bool mIsFpsCamera = false;

    // args
    bool mSnapshotMode = false;
    string mOutputFilename;

    gl::GlslProgRef mGlslProg;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    melo::NodeRef mScene;
    melo::NodeRef mSkyNode;
    melo::DirectionalLightNode::Ref mLightNode;
    melo::NodeRef mGridNode;

    melo::NodeRef mPickedNode, mMouseHitNode;
    mat4 mPickedTransform;

    FileWatcher mFileWatcher;

    shared_ptr<ui::DearLogger>  mUiLogger;

    void createDefaultScene()
    {
        mScene = melo::createRootNode();

        mSkyNode = melo::createSkyNode(RADIANCE_TEX);
        mScene->addChild(mSkyNode);

        mGridNode = melo::createGridNode(100.0f);
        mScene->addChild(mGridNode);

        mScene->addChild(melo::createMeshNode("Cube"));

        mLightNode = melo::DirectionalLightNode::create(1, { 0.5, 0.5, 0.5 });
        mLightNode->setPosition({ 10,10,10 });
        mScene->addChild(mLightNode);
    }

    void drawGUI()
    {
        mUiLogger->Draw("Log");

        if (ui::Begin("Scene"))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ui::BeginTabBar("SceneTab", tab_bar_flags))
            {
                if (ui::BeginTabItem("Hierachy"))
                {
                    auto path = getAppPath() / "melo.scene";
                    if (ui::Button("New"))
                    {
                        createDefaultScene();
                        setPickedNode(nullptr);
                    }

                    if (ui::Button("Load"))
                    {
                        auto newScene = melo::loadScene(path.generic_string());
                        if (newScene)
                        {
                            mScene = newScene;
                            setPickedNode(nullptr);
                        }
                    }

                    if (ui::Button("Save"))
                    {
                        melo::writeScene(mScene, path.generic_string());
                    }

                    ui::Separator();

                    // selectable list
                    ui::SetNextItemOpen(true, ImGuiCond_Once);
                    applyTreeUI(mScene);
                    ui::EndTabItem();
                }

                if (ui::BeginTabItem("Settings"))
                {
                    vnm::drawMinicofigImgui();
                    ui::EndTabItem();
                }

                ui::EndTabBar();
            }
        }
        ui::End();

        if (ui::Begin("Node"))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ui::BeginTabBar("NodeTab", tab_bar_flags))
            {
                if (ui::BeginTabItem("Property"))
                {
                    //ui::ScopedGroup group;
                    if (!mIsFpsCamera && mPickedNode != nullptr)
                    {
                        ui::Text(mPickedNode->getName().c_str());
                        bool isVisible = mPickedNode->isVisible();
                        if (ui::Checkbox("Visible", &isVisible))
                        {
                            mPickedNode->setVisible(isVisible);
                        }
                        if (ui::Button("Reset Transform"))
                        {
                            mPickedTransform = {};
                            mPickedNode->setTransform({});
                        }
                        ui::SameLine();
                        if (ui::Button("DEL"))
                        {
                            dispatchAsync([&] {
                                mScene->removeChild(mPickedNode);
                                setPickedNode(nullptr);
                                });
                        }
                        if (ui::Button("CLONE"))
                        {
                            dispatchAsync([&] {
                                // TODO: 
                                auto cloned = melo::create(mPickedNode->getName());
                                vec3 T, R, S;
                                ui::DecomposeMatrixToComponents(mPickedTransform, T, R, S);
                                cloned->setPosition(T);
                                cloned->setRotation(R);
                                cloned->setScale(S);
                                cloned->setTransform(mPickedTransform);
                                mScene->addChild(cloned);
                                setPickedNode(cloned);
                                });
                        }
                        if (ui::EditGizmo(mCurrentCam->getViewMatrix(), mCurrentCam->getProjectionMatrix(), &mPickedTransform))
                        {
                            mMayaCamUi.disable();
                            vec3 T, R, S;
                            ui::DecomposeMatrixToComponents(mPickedTransform, T, R, S);
                            mPickedNode->setPosition(T);
                            mPickedNode->setRotation(R);
                            mPickedNode->setScale(S);
                            mPickedNode->setTransform(mPickedTransform);
                        }
                        else
                        {
                            mMayaCamUi.enable();
                        }
                    }
                    ui::EndTabItem();
                }
                ui::EndTabBar();
            }
        }
        ui::End();
    }

    vector<string> listGlTFFiles()
    {
        vector<string> files;
        auto assetModel = (getAppPath() / "../assets").generic_string();
        for (auto& p :
            fs::recursive_directory_iterator(assetModel
#ifdef CINDER_MSW_DESKTOP
                ,
                fs::directory_options::follow_directory_symlink
#endif
            ))
        {
            auto ext = p.path().extension();
            if (ext == ".gltf" || ext == ".glb" || ext == ".obj")
            {
                auto filename = p.path().generic_string();
                filename.replace(filename.find(assetModel),
                    assetModel.size() + 1,
                    ""); // Left trim the assets prefix

                files.push_back(filename);
            }
        }

        return files;
    }

    void setPickedNode(melo::NodeRef newNode)
    {
        mPickedNode = newNode;
        if (newNode)
        {
            mPickedTransform = newNode->getTransform();
        }
    }

    void applyTreeUI(const melo::NodeRef& node)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        flag |= node->getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None;
        if (node == mPickedNode)
            flag |= ImGuiTreeNodeFlags_Selected;

        if (ui::TreeNodeEx(node->getName().c_str(), flag))
        {
            if (ui::IsItemClicked())
                setPickedNode(node);
            for (auto& child : node->getChildren())
                applyTreeUI(child);
            ui::TreePop();
        }
        else
        {
            if (ui::IsItemClicked())
                setPickedNode(node);
        }
    };

    void setup() override
    {
        log::makeLogger<log::LoggerFileRotating>(fs::path(), "IG.%Y.%m.%d.log");
        mUiLogger = log::makeLogger<ui::DearLogger>();

        am::addAssetDirectory(getAppPath() / "../assets");
        am::addAssetDirectory(getAppPath() / "../../assets");
        am::addAssetDirectory(getAppPath() / "../../../assets");

        mMayaCam.lookAt({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z }, { CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z }, vec3(0, 1, 0));
        mFpsCam.lookAt({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z }, { CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z }, vec3(0, 1, 0));
        mFpsCam.setEyePoint({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z });
        mFpsCam.setViewDirection({ CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z });
        mMayaCamUi = CameraUi(&mMayaCam, getWindow(), -1);
        mFpsCam.setup();

        createDefaultScene();

        mMeshFilenames = listGlTFFiles();
        parseArgs();

        createConfigImgui(getWindow(), false);
        //ADD_ENUM_TO_INT(mParams.get(), MESH_FILE_ID, mMeshFilenames);
        //mParams->addParam("MESH_ROTATION", &mMeshRotation);
        gl::enableDepth();
        gl::context()->depthFunc(GL_LEQUAL);

        getSignalCleanup().connect([&] { writeConfig(); });

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mMayaCam.setAspectRatio(getWindowAspectRatio());
            });

        getWindow()->getSignalMouseMove().connect([&](MouseEvent& event) {
            mMouseHitNode = pick(mScene, *mCurrentCam, event.getPos());
            });

        getWindow()->getSignalMouseUp().connect([&](MouseEvent& event) {
            if (event.isLeft()) {
                auto hit = mMouseHitNode;
                dispatchAsync([&, hit] {
                    setPickedNode(hit);
                    });
            }
            });
        if (!mSnapshotMode)
        {
            getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
                auto code = event.getCode();
                if (code == KeyEvent::KEY_ESCAPE)
                    quit();
                if (code == KeyEvent::KEY_SPACE)
                    WIRE_FRAME = !WIRE_FRAME;
                if (code == KeyEvent::KEY_e)
                    ENV_VISIBLE = !ENV_VISIBLE;
                if (code == KeyEvent::KEY_x)
                    XYZ_VISIBLE = !XYZ_VISIBLE;
                if (code == KeyEvent::KEY_g)
                    GUI_VISIBLE = !GUI_VISIBLE;
                if (code == KeyEvent::KEY_f)
                    FPS_CAMERA = !FPS_CAMERA;
                });
        }

        getWindow()->getSignalFileDrop().connect([&](FileDropEvent& event) {

            static auto imageExts = ImageIo::getLoadExtensions();

            for (auto& filePath : event.getFiles())
            {
                if (fs::is_directory(filePath)) continue;
                if (!filePath.has_extension()) continue;
                auto ext = filePath.extension().string().substr(1);

                if (ext == "obj" || ext == "gltf" || ext == "glb")
                {
                    dispatchAsync([&, filePath] {
                        loadMeshFromFile(filePath);
                        });
                    break;
                }

                bool isImageType = std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end();
                if (isImageType)
                {
                    TEX0_NAME = filePath.string();
                    break;
                }
            }
            });

        getSignalUpdate().connect([&] {

            if (mIsFpsCamera != FPS_CAMERA)
            {
                if (FPS_CAMERA)
                {
                    mFpsCam.setActive(true);
                    mFpsCam.setEyePoint(mMayaCam.getEyePoint());
                    mFpsCam.setViewDirection(mMayaCam.getViewDirection());
                    //mFpsCam.look = mMayaCam.getPivotPoint();
                }
                else
                {
                    mFpsCam.setActive(false);
                    mMayaCam.lookAt(mFpsCam.getEyePoint(), mMayaCam.getPivotPoint());
                    mMayaCam.setViewDirection(mFpsCam.getViewDirection());
                    mMayaCam.setWorldUp(mFpsCam.getWorldUp());
                }
                mIsFpsCamera = FPS_CAMERA;
            }

            if (FPS_CAMERA)
            {
                mCurrentCam = &mFpsCam;
            }
            else
            {
                mCurrentCam = &mMayaCam;
            }
            CAM_POS_X = mCurrentCam->getEyePoint().x;
            CAM_POS_Y = mCurrentCam->getEyePoint().y;
            CAM_POS_Z = mCurrentCam->getEyePoint().z;
            CAM_DIR_X = mCurrentCam->getViewDirection().x;
            CAM_DIR_Y = mCurrentCam->getViewDirection().y;
            CAM_DIR_Z = mCurrentCam->getViewDirection().z;
            mCurrentCam->setNearClip(CAM_Z_NEAR);
            mCurrentCam->setFarClip(CAM_Z_FAR);

            for (auto& child : mScene->getChildren())
            {
                //mModel->flipV = FLIP_V;
                child->cameraPosition = mCurrentCam->getEyePoint();
                child->lightDirection = glm::normalize(mLightNode->getPosition());
                child->lightColor = mLightNode->color;
            }

            if (GUI_VISIBLE)
            {
                drawGUI();
            }

            if (!mIsFpsCamera && mPickedNode != nullptr)
            {
                auto k = 128;
                auto pos = ivec2(getWindowWidth() - k, 0);
                auto size = ivec2(k, k);
                if (ui::ViewManipulate(mCurrentCam->getViewMatrixReference(), 8, pos, size))
                {
                    mMayaCamUi.disable();
                }
                else
                {
                    mMayaCamUi.enable();
                }
            }

            mScene->treeUpdate();
            });

        getWindow()->getSignalDraw().connect([&] {
            if (mIsFpsCamera)
                gl::setMatrices(mFpsCam);
            else
                gl::setMatrices(mMayaCam);
            if (mSnapshotMode)
                gl::clear(ColorA::gray(0.0f, 0.0f));
            else
                gl::clear(ColorA::gray(0.2f, 1.0f));

            mSkyNode->setVisible(ENV_VISIBLE);
            mGridNode->setVisible(XYZ_VISIBLE);

            gl::setWireframeEnabled(WIRE_FRAME);
            mScene->treeDraw();
            gl::disableWireframe();

            if (mMouseHitNode)
            {
                melo::drawBoundingBox(mMouseHitNode);
            }

            if (mPickedNode)
            {
                melo::drawBoundingBox(mPickedNode, Color(1, 0, 0));
            }

            if (mSnapshotMode)
            {
                auto windowSurf = copyWindowSurfaceWithAlpha();
                writeImage(mOutputFilename, windowSurf);
                quit();
            }
            });
    }

    void loadMeshFromFile(fs::path path)
    {
        if (melo::Node::radianceTexture == nullptr)
        {
            melo::Node::radianceTexture = am::textureCubeMap(RADIANCE_TEX);
            melo::Node::irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
            melo::Node::brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);
        }

        auto newModel = melo::createMeshNode(path);
        if (newModel)
        {
            mScene->addChild(newModel);
        }
    }

    void parseArgs()
    {
        auto& args = getCommandLineArgs();

        if (args.size() > 1)
        {
            // /path/to/MeloViewer.exe file.obj
            auto filePath = args[1];
            dispatchAsync([&, filePath] {
                loadMeshFromFile(filePath);
                });
            if (args.size() > 2)
            {
                // MeloViewer.exe file.obj snapshot.png
                mSnapshotMode = true;
                GUI_VISIBLE = false;
                WIRE_FRAME = false;
                mOutputFilename = args[2];

                if (args.size() > 3)
                {
                    // MeloViewer.exe file.obj snapshot.png new_shining_texture.png
                    TEX0_NAME = args[3];
                }
            }
        }
    }
};

void preSettings(App::Settings* settings)
{
    readConfig();
#if defined( CINDER_MSW_DESKTOP )
    settings->setConsoleWindowEnabled(CONSOLE_ENABLED);
#endif
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
}

#if !defined(NDEBUG) && defined(CINDER_MSW)
auto gfxOption = RendererGl::Options().msaa(4).debug().debugLog(GL_DEBUG_SEVERITY_MEDIUM);
#else
auto gfxOption = RendererGl::Options().msaa(4);
#endif
CINDER_APP(MeloViewer, RendererGl(gfxOption), preSettings)
