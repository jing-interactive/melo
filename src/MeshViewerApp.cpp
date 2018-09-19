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

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera/flythrough_camera.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct FlythroughCamera : public CameraPersp
{
    double mElapsedSeconds;
    ivec2 mMousePos, mPrevMousePos;

    bool mIsKeyPressed[KeyEvent::KEY_LAST] = {false};
    bool mIsRMousePressed = false;

    vec3 pos = {3.0f, 3.0f, 3.0f};
    vec3 look = {0.0f, 0.0f, 1.0f};
    const vec3 up = {0.0f, 1.0f, 0.0f};
    float degrees_per_cursor_move = 0.5f;
    float max_pitch_rotation_degrees = 80.0f;
    
    bool activated = true;

    void setup()
    {
        mElapsedSeconds = getElapsedSeconds();
        //        mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());

//        flythrough_camera_look_to(pos, look, up, glm::value_ptr(mViewMatrix), 0);

        getWindow()->getSignalResize().connect([&] { setAspectRatio(getWindowAspectRatio()); });

        getWindow()->getSignalKeyDown().connect(
            [&](KeyEvent& event) { mIsKeyPressed[event.getCode()] = true; });

        getWindow()->getSignalKeyUp().connect(
            [&](KeyEvent& event) { mIsKeyPressed[event.getCode()] = false; });

        getWindow()->getSignalMouseDown().connect(
            [&](MouseEvent& ev) { mIsRMousePressed = ev.isRight(); });

        getWindow()->getSignalMouseUp().connect([&](MouseEvent& ev) { mIsRMousePressed = false; });

        getWindow()->getSignalMouseMove().connect([&](MouseEvent& ev) {
            mIsRMousePressed = ev.isRight();
            mMousePos = ev.getPos();
        });

        AppBase::get()->getSignalUpdate().connect([&] {
            float delta_time_sec = getElapsedSeconds() - mElapsedSeconds;
            mElapsedSeconds = getElapsedSeconds();

            flythrough_camera_update(
                glm::value_ptr(pos), glm::value_ptr(look), glm::value_ptr(up),
                glm::value_ptr(mViewMatrix), delta_time_sec,
                1.0f * (mIsKeyPressed[KeyEvent::KEY_LSHIFT] ? 2.0f : 1.0f) * activated,
                degrees_per_cursor_move, max_pitch_rotation_degrees, mMousePos.x - mPrevMousePos.x,
                mMousePos.y - mPrevMousePos.y, mIsKeyPressed[KeyEvent::KEY_w],
                mIsKeyPressed[KeyEvent::KEY_a], mIsKeyPressed[KeyEvent::KEY_s],
                mIsKeyPressed[KeyEvent::KEY_d], mIsKeyPressed[KeyEvent::KEY_SPACE],
                mIsKeyPressed[KeyEvent::KEY_LCTRL], 0);

            if (activated)
            {
                float* view = glm::value_ptr(mViewMatrix);
                printf("\n");
                printf("pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
                printf("look: %f, %f, %f\n", look[0], look[1], look[2]);
                printf("view: %f %f %f %f\n"
                       "      %f %f %f %f\n"
                       "      %f %f %f %f\n"
                       "      %f %f %f %f\n",
                       view[0], view[1], view[2], view[3], view[4], view[5], view[6], view[7],
                       view[8], view[9], view[10], view[11], view[12], view[13], view[14],
                       view[15]);
                mModelViewCached = true;
            }
            mPrevMousePos = mMousePos;
        });
    }
};
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

    FlythroughCamera mFlyCam;

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

        mFlyCam.setup();

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
#if 0
            gl::setMatrices(mCam);
#else
            gl::setMatrices(mFlyCam);
#endif
            gl::clear();

            if (XYZ_VISIBLE)
            {
                gl::drawCoordinateFrame(10, 0.1, 0.01);
            }

            if (mRootGLTF)
            {
                gl::ScopedTextureBind scpTex(mRootGLTF->radianceTexture, 0);
                mSkyBoxBatch->draw();

                gl::setWireframeEnabled(WIRE_FRAME);
                mRootGLTF->currentScene->setScale(MESH_SCALE);
                mRootGLTF->draw();
                gl::disableWireframe();
            }

            if (mRootObjRef)
            {
                gl::setWireframeEnabled(WIRE_FRAME);
                mRootObjRef->setScale(MESH_SCALE);
                mRootObjRef->treeDraw();
                gl::disableWireframe();
            }
        });
    }
};

#ifndef NDEBUG
auto gfxOption = RendererGl::Options().msaa(4).debug().debugLog(GL_DEBUG_SEVERITY_MEDIUM);
#else
auto gfxOption = RendererGl::Options().msaa(4);
#endif
CINDER_APP(MeshViewerApp, RendererGl(gfxOption), [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
