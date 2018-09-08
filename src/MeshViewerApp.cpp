#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include "Cinder-Nodes/include/Node3D.h"

#include "cigltf.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct MeshViewerApp : public App
{
    RootGLTFRef mRootGLTF;

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        mRootGLTF = RootGLTF::create(getAssetPath(MESH_NAME));

        // mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        createConfigUI({400, 300});
        gl::enableDepth();

        getWindow()->getSignalResize().connect(
            [&] { mCam.setAspectRatio(getWindowAspectRatio()); });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE)
                quit();
        });


        getSignalUpdate().connect([&] {
            mRootGLTF->flipV = FLIP_V;
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

            gl::setWireframeEnabled(WIRE_FRAME);
            mRootGLTF->draw();
            gl::disableWireframe();
        });
    }

  private:
    CameraPersp mCam;
    CameraUi mCamUi;
    gl::GlslProgRef mGlslProg;
};

CINDER_APP(MeshViewerApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
