#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "cinder/Arcball.h"
#include "cinder/params/Params.h"

#include "CinderMeshViewer.h"

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera/flythrough_camera.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined( CINDER_GL_ES )
namespace cinder {
    namespace gl {
        void enableWireframe() {}
        void disableWireframe() {}
        void setWireframeEnabled(bool enable = true) { if (enable) enableWireframe(); else disableWireframe(); }
    }
}
#endif

gl::VertBatchRef createGrid()
{
    auto grid = gl::VertBatch::create(GL_LINES);
    grid->begin(GL_LINES);
    float scale = 1.0f;
    for (int i = -10; i <= 10; ++i)
    {
        grid->color(Color(0.25f, 0.25f, 0.25f));
        grid->color(Color(0.25f, 0.25f, 0.25f));
        grid->color(Color(0.25f, 0.25f, 0.25f));
        grid->color(Color(0.25f, 0.25f, 0.25f));

        grid->vertex(float(i)*scale, 0.0f, -10.0f*scale);
        grid->vertex(float(i)*scale, 0.0f, +10.0f*scale);
        grid->vertex(-10.0f*scale, 0.0f, float(i)*scale);
        grid->vertex(+10.0f*scale, 0.0f, float(i)*scale);
    }
    grid->end();

    return grid;
}

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
    quat mMeshRotation;

    gl::GlslProgRef mGlslProg;
    gl::BatchRef mSkyBoxBatch;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    // 1 of 3
    RootGLTFRef mRootGLTF;
    RootObjRef mRootObj;
    gl::VboMeshRef mVboMesh;

    gl::VertBatchRef mGrid;

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
        addAssetDirectory(getAppPath() / "../../../assets");

        mFlyCam.setup();

        auto skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
        if (!skyBoxShader)
        {
            quit();
            return;
        }
        skyBoxShader->uniform("uCubeMapTex", 0);
        skyBoxShader->uniform("uExposure", 2.0f);
        skyBoxShader->uniform("uGamma", 2.0f);

        mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(400)), skyBoxShader);
        mGrid = createGrid();

        mCam.lookAt({CAM_POS_X, CAM_POS_Y, CAM_POS_Z}, vec3(), vec3(0, 1, 0));
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        mMeshFilenames = listGlTFFiles();
#ifndef CINDER_COCOA_TOUCH
        auto params = createConfigUI({400, 500});
        ADD_ENUM_TO_INT(params.get(), MESH_FILE_ID, mMeshFilenames);
        params->addParam("MESH_ROTATION", &mMeshRotation);
#endif
        gl::enableDepth();

        RootGLTF::radianceTexture = am::textureCubeMap(RADIANCE_TEX);
        RootGLTF::irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
        RootGLTF::brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mCam.setAspectRatio(getWindowAspectRatio());
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            auto code = event.getCode();
            if (code == KeyEvent::KEY_ESCAPE)
                quit();
            if (code == KeyEvent::KEY_w)
                WIRE_FRAME = !WIRE_FRAME;
            if (code == KeyEvent::KEY_e)
                ENV_VISIBLE = !ENV_VISIBLE;
            if (code == KeyEvent::KEY_x)
                XYZ_VISIBLE = !XYZ_VISIBLE;
        });

        getWindow()->getSignalFileDrop().connect([&](FileDropEvent& event) {

            static auto imageExts = ImageIo::getLoadExtensions();

            for (auto& filePath : event.getFiles())
            {
                if (fs::is_directory(filePath)) continue;
                if (!filePath.has_extension()) continue;

                if (filePath.extension() == ".obj" || filePath.extension() == ".gltf" || filePath.extension() == ".glb")
                {
                    mMeshFilenames.emplace_back(filePath.string());
                    MESH_FILE_ID = mMeshFilenames.size() - 1;
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
                mVboMesh.reset();
                mRootObj.reset();
                mRootGLTF.reset();

                if (path.extension() == ".obj")
                {
                    if (AM_VBO_MESH)
                    {
                        mVboMesh = am::vboMesh(path.string());
                    }
                    else
                    {
                        mRootObj = RootObj::create(path);
                    }
                }
                else
                {
                    mRootGLTF = RootGLTF::create(path);
                }

                if (!mRootObj && !mVboMesh && !mRootGLTF)
                {
                    mVboMesh = am::vboMesh("Teapot");
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
#if 1
            gl::setMatrices(mCam);
#else
            gl::setMatrices(mFlyCam);
#endif
            gl::clear();

            if (mRootGLTF)
            {
                gl::ScopedTextureBind scpTex(mRootGLTF->radianceTexture, 0);
                if (ENV_VISIBLE)
                {
                    gl::ScopedDepthWrite depthWrite(false);
                    mSkyBoxBatch->draw();
                }

                gl::setWireframeEnabled(WIRE_FRAME);
                mRootGLTF->currentScene->setScale(MESH_SCALE);
                mRootGLTF->currentScene->setRotation(mMeshRotation);
                mRootGLTF->draw();
                gl::disableWireframe();
            }

            if (mRootObj)
            {
                gl::setWireframeEnabled(WIRE_FRAME);
                mRootObj->setScale(MESH_SCALE);
                mRootObj->setRotation(mMeshRotation);
                mRootObj->treeDraw();
                gl::disableWireframe();
            }

            if (mVboMesh)
            {
                gl::ScopedGlslProg glsl(am::glslProg("lambert texture"));
                gl::ScopedTextureBind tex0(am::texture2d("checkerboard"));
                gl::setWireframeEnabled(WIRE_FRAME);
                gl::ScopedModelMatrix mtx;
                gl::scale(MESH_SCALE, MESH_SCALE, MESH_SCALE);
                gl::rotate(mMeshRotation);
                gl::draw(mVboMesh);
                gl::disableWireframe();
            }

            if (XYZ_VISIBLE)
            {
                gl::ScopedGlslProg glsl(am::glslProg("color"));
                mGrid->draw();
                gl::ScopedDepthTest depthTest(false);
                gl::drawCoordinateFrame(10, 0.5, 0.1);
            }
        });
    }
};

#if !defined(NDEBUG) && defined(CINDER_MSW)
auto gfxOption = RendererGl::Options().msaa(4).debug().debugLog(GL_DEBUG_SEVERITY_MEDIUM);
#else
auto gfxOption = RendererGl::Options().msaa(4);
#endif
CINDER_APP(MeshViewerApp, RendererGl(gfxOption), [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
