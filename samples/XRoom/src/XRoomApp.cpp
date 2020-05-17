#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/json.h"

// vnm
#include "AssetManager.h"
#include "MiniConfig.h"
#include "FontHelper.h"
#include "GLHelper.h"

// melo
#include "melo.h"
#include "NodeExt.h"
#include "ciobj.h"

// imgui
#include "MiniConfigImgui.h"
#include "CinderGuizmo.h"
#include "DearLogger.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct XRoomApp : public App
{
    CameraPersp mCam;
    CameraUi mCamUi;

    // args

    gl::GlslProgRef mGlslProg;

    melo::NodeRef mScene;
    melo::NodeRef mSkyNode;
    melo::DirectionalLightNode::Ref mLightNode;
    melo::NodeRef mGridNode;

    shared_ptr<ImGui::DearLogger>  mUiLogger;

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

        auto obj_3d = JsonTree(app::loadAsset(OBJ_3D_JSON));
        auto children = obj_3d.getChildren();
        int count = std::min<int>(100, children.size());
        float spacing = 15;
        int idx = 0;
        for (const auto& sku : children)
        {
            auto name = sku.getValueForKey("mesh");
            //CI_LOG_W(name);

            auto node = ModelObj::create(getAssetPath(name));
            int x = idx / 10;
            int y = idx % 10;
            node->setPosition({0, x * spacing, y * spacing});
            mScene->addChild(node);

            if (++idx >= count) break;
        }
    }

    void drawGUI()
    {
        vnm::drawMinicofigImgui();

        mUiLogger->Draw("Log");
    }

    void setup() override
    {
        log::makeLogger<log::LoggerFileRotating>(fs::path(), "IG.%Y.%m.%d.log");
        mUiLogger = log::makeLogger<ImGui::DearLogger>();

        am::addAssetDirectory(getAppPath() / "../assets");
        am::addAssetDirectory(getAppPath() / "../../assets");
        am::addAssetDirectory(getAppPath() / "../../../assets");
        am::addAssetDirectory(GC_RAW_FOLDER);

        mCam.lookAt({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z }, { CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z }, vec3(0, 1, 0));
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        createDefaultScene();

        parseArgs();

        createConfigImgui(getWindow(), false);
        //ADD_ENUM_TO_INT(mParams.get(), MESH_FILE_ID, mMeshFilenames);
        //mParams->addParam("MESH_ROTATION", &mMeshRotation);
        gl::enableDepth();
        gl::context()->depthFunc(GL_LEQUAL);

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            auto code = event.getCode();
            switch (code)
            {
            case KeyEvent::KEY_ESCAPE:
                quit(); break;
            case KeyEvent::KEY_w:
                WIRE_FRAME = !WIRE_FRAME; break;
            case KeyEvent::KEY_e:
                ENV_VISIBLE = !ENV_VISIBLE; break;
            case KeyEvent::KEY_x:
                XYZ_VISIBLE = !XYZ_VISIBLE; break;
            case KeyEvent::KEY_g:
                GUI_VISIBLE = !GUI_VISIBLE; break;
            }

            });
        getSignalCleanup().connect([&] { writeConfig(); });

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mCam.setAspectRatio(getWindowAspectRatio());
            });

        getWindow()->getSignalMouseDown().connect([&](MouseEvent& event) {
            });

        getWindow()->getSignalMouseDrag().connect([&](MouseEvent& event) {
            });

        getWindow()->getSignalMouseMove().connect([&](MouseEvent& event) {
            });

        getWindow()->getSignalMouseUp().connect([&](MouseEvent& event) {
            });

        getWindow()->getSignalFileDrop().connect([&](FileDropEvent& event) {

            });

        getSignalUpdate().connect([&] {

            CAM_POS_X = mCam.getEyePoint().x;
            CAM_POS_Y = mCam.getEyePoint().y;
            CAM_POS_Z = mCam.getEyePoint().z;
            CAM_DIR_X = mCam.getViewDirection().x;
            CAM_DIR_Y = mCam.getViewDirection().y;
            CAM_DIR_Z = mCam.getViewDirection().z;
            mCam.setNearClip(CAM_Z_NEAR);
            mCam.setFarClip(CAM_Z_FAR);

            for (auto& child : mScene->getChildren())
            {
                //mModel->flipV = FLIP_V;
                child->cameraPosition = mCam.getEyePoint();
                child->lightDirection = glm::normalize(mLightNode->getPosition());
                child->lightColor = mLightNode->color;
            }

            if (GUI_VISIBLE)
            {
                drawGUI();
            }

            mScene->treeUpdate();
            });

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices(mCam);
            gl::clear(ColorA::gray(0.2f, 1.0f));

            mSkyNode->setVisible(ENV_VISIBLE);
            mGridNode->setVisible(XYZ_VISIBLE);

            gl::setWireframeEnabled(WIRE_FRAME);
            mScene->treeDraw();
            gl::disableWireframe();

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
            // /path/to/XRoomApp.exe file.obj
            auto filePath = args[1];
            dispatchAsync([&, filePath] {
                loadMeshFromFile(filePath);
                });
            if (args.size() > 2)
            {
                // XRoomApp.exe file.obj snapshot.png
                GUI_VISIBLE = false;
                WIRE_FRAME = false;

                if (args.size() > 3)
                {
                    // XRoomApp.exe file.obj snapshot.png new_shining_texture.png
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
CINDER_APP(XRoomApp, RendererGl(gfxOption), preSettings)
