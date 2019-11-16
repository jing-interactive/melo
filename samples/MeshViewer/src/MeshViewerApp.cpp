#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ObjLoader.h"
#include "cinder/params/Params.h"
#include "AssetManager.h"
#include "MiniConfig.h"
#include "FontHelper.h"
#include "GLHelper.h"

#include "melo.h"
#include "FirstPersonCamera.h"

#include "MiniConfigImgui.h"
#include "CinderGuizmo.h"

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
    quat mMeshRotation;

    // args
    bool mSnapshotMode = false;
    string mOutputFilename;

    gl::GlslProgRef mGlslProg;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    nodes::Node3DRef mScene;
    nodes::Node3DRef mModel;
    nodes::Node3DRef mSkyNode;
    nodes::Node3DRef mGridNode;

    nodes::Node3DRef mPickedNode;
    mat4 mPickedTransform;


    string mLoadingError;
    gl::TextureFontRef texFont;
    //params::InterfaceGlRef mParams;

    vector<string> listGlTFFiles()
    {
        vector<string> files;
        auto assetModel = getAppPath() / "../assets";
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
                filename.replace(filename.find(assetModel.generic_string()),
                    assetModel.generic_string().size(),
                    ""); // Left trim the assets prefix

                files.push_back(filename);
            }
        }

        return files;
    }

    void setup() override
    {
        log::makeLogger<log::LoggerFileRotating>(fs::path(), "IG.%Y.%m.%d.log");
        am::addAssetDirectory(getAppPath() / "../assets");
        am::addAssetDirectory(getAppPath() / "../../assets");
        am::addAssetDirectory(getAppPath() / "../../../assets");

        mMayaCam.lookAt({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z }, vec3(), vec3(0, 1, 0));
        mMayaCamUi = CameraUi(&mMayaCam, getWindow(), -1);
        mFpsCam.setup();

        texFont = FontHelper::createTextureFont("Helvetica", 24);

        {
            mScene = melo::createSceneRoot();

            mSkyNode = melo::createSky();
            mScene->addChild(mSkyNode);

            mGridNode = melo::createGrid(100.0f);
            mScene->addChild(mGridNode);
        }

        mMeshFilenames = listGlTFFiles();
        parseArgs();

        createConfigImgui();
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

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (mSnapshotMode) return;

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

        getWindow()->getSignalFileDrop().connect([&](FileDropEvent& event) {

            static auto imageExts = ImageIo::getLoadExtensions();

            for (auto& filePath : event.getFiles())
            {
                if (fs::is_directory(filePath)) continue;
                if (!filePath.has_extension()) continue;
                auto ext = filePath.extension();

                if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
                {
                    mMeshFilenames.emplace_back(filePath.string());
                    MESH_FILE_ID = mMeshFilenames.size() - 1;
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
            if (MESH_FILE_ID > mMeshFilenames.size() - 1)
                MESH_FILE_ID = 0;

            if (mMeshFileId != MESH_FILE_ID)
            {
                mMeshFileId = MESH_FILE_ID;

                fs::path path = mMeshFilenames[mMeshFileId];
                if (!fs::exists(path))
                {
                    path = getAssetPath(mMeshFilenames[mMeshFileId]);
                }

                if (nodes::Node3D::radianceTexture == nullptr)
                {
                    nodes::Node3D::radianceTexture = am::textureCubeMap(RADIANCE_TEX);
                    nodes::Node3D::irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
                    nodes::Node3D::brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);
                }

                auto newModel = melo::createFromFile(path);
                if (newModel)
                {
                    auto box = ci::AxisAlignedBox(newModel->mBoundBoxMin, newModel->mBoundBoxMax);
                    mMayaCam.lookAt(box.getMax(), box.getCenter());
                    mScene->removeChild(mModel);
                    mModel = newModel;
                    mScene->addChild(mModel);
                    mPickedNode = mModel;
                }
            }

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
            CAM_POS_X = mMayaCam.getEyePoint().x;
            CAM_POS_Y = mMayaCam.getEyePoint().y;
            CAM_POS_Z = mMayaCam.getEyePoint().z;
            mCurrentCam->setNearClip(CAM_Z_NEAR);
            mCurrentCam->setFarClip(CAM_Z_FAR);

            if (mModel)
            {
                //mModel->flipV = FLIP_V;
                mModel->cameraPosition = mCurrentCam->getEyePoint();
                mModel->treeUpdate();
            }

            if (ui::Begin("Scene Inspector"))
            {
                // selectable list
                if (ui::ListBoxHeader("Nodes"))
                {
                    static bool selected = false;
                    for (auto node : mScene->getChildren<nodes::Node3D>())
                    {
                        if (ui::Selectable(node->getName().c_str(), mPickedNode == node))
                        {
                            mPickedNode = node;
                            mPickedTransform = mPickedNode->getTransform();
                        }
                    }
                    ui::ListBoxFooter();
                }

                ui::ScopedGroup group;
                if (!mIsFpsCamera && mPickedNode != nullptr)
                {
                    bool isVisible = mPickedNode->isVisible();
                    if (ui::Checkbox("Visible", &isVisible))
                    {
                        mPickedNode->setVisible(isVisible);
                    }
                    if (ui::Button("Reset Transform"))
                    {
                        mPickedTransform = {};
                        mPickedNode->setTransform(mPickedTransform);
                    }
                    if (ui::EditGizmo(mCurrentCam->getViewMatrix(), mCurrentCam->getProjectionMatrix(), &mPickedTransform))
                    {
                        mMayaCamUi.disable();
                        mPickedNode->setTransform(mPickedTransform);
                    }
                    else
                    {
                        mMayaCamUi.enable();
                    }
                }
            }
            ui::End();

            ui::ViewManipulate(mCurrentCam->getViewMatrixReference(), 8, { getWindowWidth() - 128, 0 }, { 128, 128 });
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

            //mParams->show(GUI_VISIBLE);

            if (!mLoadingError.empty())
            {
                texFont->drawString(mLoadingError, { 10, APP_HEIGHT - 150 });
            }

            mSkyNode->setVisible(ENV_VISIBLE);
#if 0
            if (mModel)
            {
                gl::ScopedTextureBind scpTex(mModel->radianceTexture, 0);

                gl::pointSize(POINT_SIZE);

                gl::setWireframeEnabled(WIRE_FRAME);
                mModel->setScale({ MESH_SCALE,MESH_SCALE,MESH_SCALE });
                mModel->setRotation(mMeshRotation);
                mModel->treeDraw();
                gl::disableWireframe();
            }
#endif
            mGridNode->setVisible(XYZ_VISIBLE);

            mScene->treeDraw();

            if (mSnapshotMode)
            {
                auto windowSurf = copyWindowSurfaceWithAlpha();
                fs::path writePath = mOutputFilename;
                writeImage(writePath, windowSurf);
                quit();
            }
            });
    }
    void parseArgs()
    {
        auto& args = getCommandLineArgs();

        if (args.size() > 1)
        {
            // MeloViewer.exe file.obj
            MESH_FILE_ID = mMeshFilenames.size();
            mMeshFilenames.push_back(args[1]);

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

#if !defined(NDEBUG) && defined(CINDER_MSW)
auto gfxOption = RendererGl::Options().msaa(4).debug().debugLog(GL_DEBUG_SEVERITY_MEDIUM);
#else
auto gfxOption = RendererGl::Options().msaa(4);
#endif
CINDER_APP(MeloViewer, RendererGl(gfxOption), [](App::Settings* settings) {
    readConfig();
    settings->setConsoleWindowEnabled(CONSOLE_ENABLED);
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
    })
