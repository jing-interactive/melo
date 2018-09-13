#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "cinder/Arcball.h"
#include "cinder/params/Params.h"

#include "cigltf.h"
#include "ciobj.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct MeshViewerApp : public App
{
    CameraPersp mCam;
    CameraUi mCamUi;
    Arcball mArcball;

    gl::GlslProgRef mGlslProg;
    gl::BatchRef mSkyBoxBatch;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    RootGLTFRef mRootGLTF;
    RootObjRef mRootObjRef;

    vector<string> listGlTFFiles()
    {
        vector<string> files;
        auto assetRoot = getAssetPath("");
        for (auto& p :
             fs::recursive_directory_iterator(assetRoot
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
                filename.replace(filename.find(assetRoot.generic_string()),
                                 assetRoot.generic_string().size(),
                                 ""); // Left trim the assets prefix

                files.push_back(filename);
            }
        }

        return files;
    }

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        auto skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
        skyBoxShader->uniform("uCubeMapTex", 0);

        mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(400)), skyBoxShader);

        mCam.lookAt({CAM_POS_X, CAM_POS_Y, CAM_POS_Z}, vec3(), vec3(0, 1, 0));
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        mMeshFilenames = listGlTFFiles();
        auto params = createConfigUI({400, 400});
        ADD_ENUM_TO_INT(params.get(), MESH_FILE_ID, mMeshFilenames);

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
            if (mMeshFileId != MESH_FILE_ID)
            {
                mMeshFileId = MESH_FILE_ID;

                auto path = getAssetPath(mMeshFilenames[mMeshFileId]);
                mRootObjRef.reset();
                mRootGLTF.reset();
                if (path.extension() == ".obj")
                {
                    mRootObjRef = RootObj::create(path);
                    if (!mRootObjRef)
                        quit();
                }
                else
                {
                    mRootGLTF = RootGLTF::create(path);
                    if (!mRootGLTF)
                        quit();
                }
            }

            CAM_POS_X = mCam.getEyePoint().x;
            CAM_POS_Y = mCam.getEyePoint().y;
            CAM_POS_Z = mCam.getEyePoint().z;
            mCam.setNearClip(CAM_Z_NEAR);
            mCam.setFarClip(CAM_Z_FAR);

            if (mRootGLTF)
            {
                mRootGLTF->flipV = FLIP_V;
                mRootGLTF->cameraPosition = mCam.getEyePoint();
                mRootGLTF->update();
            }
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices(mCam);
            gl::clear();

            if (XYZ_VISIBLE)
            {
                gl::drawCoordinateFrame(10, 0.1, 0.01);
            }

            if (mRootGLTF)
            {
                gl::ScopedTextureBind scpTex(mRootGLTF->radianceTexture, 0);
                mSkyBoxBatch->draw();

                {
                    gl::setWireframeEnabled(WIRE_FRAME);
                    if (mRootGLTF)
                    {
                        mRootGLTF->currentScene->setScale(MESH_SCALE);
                        mRootGLTF->draw();
                    }

                    gl::disableWireframe();
                }
            }

            if (mRootObjRef)
            {
                mRootObjRef->draw();
            }
        });
    }
};

CINDER_APP(MeshViewerApp, RendererGl(RendererGl::Options().msaa(4)), [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
