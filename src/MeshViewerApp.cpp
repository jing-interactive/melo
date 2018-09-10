#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include "cigltf.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct MeshViewerApp : public App
{
    CameraPersp mCam;
    CameraUi mCamUi;
    gl::GlslProgRef mGlslProg;
    gl::BatchRef mSkyBoxBatch;

    RootGLTFRef mRootGLTF;

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        mRootGLTF = RootGLTF::create(getAssetPath(MESH_NAME));
        if (!mRootGLTF) quit();

        auto skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
        skyBoxShader->uniform("uCubeMapTex", 0);

        mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(400)), skyBoxShader);

        mCam.lookAt({ CAM_POS_X ,CAM_POS_Y, CAM_POS_Z }, vec3(), vec3(0, 1, 0));
        mCam.setNearClip(0.1);
        mCam.setFarClip(1000);
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        createConfigUI({400, 400});
        gl::enableDepth();

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mCam.setAspectRatio(getWindowAspectRatio());
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE)
                quit();
        });

        getSignalUpdate().connect([&] {
            mRootGLTF->flipV = FLIP_V;
            mRootGLTF->cameraPosition = mCam.getEyePoint();
            mRootGLTF->update();
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices(mCam);
            gl::clear();

            if (XYZ_VISIBLE)
            {
                gl::drawCoordinateFrame(10, 0.1, 0.01);
            }

            {
                gl::ScopedTextureBind scpTex(mRootGLTF->radianceTexture, 0);
                mSkyBoxBatch->draw();
            }

            {
                gl::setWireframeEnabled(WIRE_FRAME);
                mRootGLTF->currentScene->setScale(MESH_SCALE);
                mRootGLTF->draw();
                gl::disableWireframe();
            }
        });
    }
};

CINDER_APP(MeshViewerApp, RendererGl(RendererGl::Options().msaa(4)), [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
