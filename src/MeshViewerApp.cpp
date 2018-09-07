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

        createConfigUI({200, 200});
        gl::enableDepth();

        getWindow()->getSignalResize().connect(
            [&] { mCam.setAspectRatio(getWindowAspectRatio()); });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE)
                quit();
        });

        mGlslProg = am::glslProg(VS_NAME, FS_NAME);
        mGlslProg->uniform("uTex0", 0);
        mGlslProg->uniform("uTex1", 1);
        mGlslProg->uniform("uTex2", 2);
        mGlslProg->uniform("uTex3", 3);

        getSignalUpdate().connect([&] { mRootGLTF->update(); });

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices(mCam);
            gl::clear();

            gl::ScopedTextureBind tex0(am::texture2d(TEX0_NAME), 0);
            gl::ScopedTextureBind tex1(am::texture2d(TEX1_NAME), 1);
            gl::ScopedTextureBind tex2(am::texture2d(TEX2_NAME), 2);
            gl::ScopedTextureBind tex3(am::texture2d(TEX3_NAME), 3);
            gl::ScopedGlslProg glsl(mGlslProg);

            mRootGLTF->draw();
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
