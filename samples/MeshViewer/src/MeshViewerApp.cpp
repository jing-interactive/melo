#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ObjLoader.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "FontHelper.h"

#include "cinder/Arcball.h"
#include "cinder/params/Params.h"

#include "CinderMeshViewer.h"
#include "FirstPersonCamera.h"

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


struct MeshViewerApp : public App
{
    CameraPersp mMayaCam;
    CameraUi mMayaCamUi;
    bool mIsFpsCamera = false;
    Arcball mArcball;
    quat mMeshRotation;

    gl::GlslProgRef mGlslProg;
    gl::BatchRef mSkyBoxBatch;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    // 1 of 3
    ModelGLTFRef mModelGLTF;
    ModelObjRef mModelObj;
    gl::VboMeshRef mVboMesh;
    TriMeshRef mTriMesh;

    gl::VertBatchRef mGrid;

    FirstPersonCamera mFpsCam;

    string mLoadingError;
    gl::TextureFontRef texFont;
    params::InterfaceGlRef mParams;

    vector<string> listGlTFFiles()
    {
        vector<string> files;
        auto assetModel = getAssetPath("");
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
        log::makeLogger<log::LoggerFile>();
        addAssetDirectory(getAppPath() / "../../../assets");

        texFont = FontHelper::createTextureFont("Helvetica", 24);

        mFpsCam.setup();

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

        mMayaCam.lookAt({CAM_POS_X, CAM_POS_Y, CAM_POS_Z}, vec3(), vec3(0, 1, 0));
        mMayaCamUi = CameraUi(&mMayaCam, getWindow(), -1);

        mMeshFilenames = listGlTFFiles();
        auto& args = getCommandLineArgs();
        if (args.size() == 2)
        {
            MESH_FILE_ID = mMeshFilenames.size();
            mMeshFilenames.push_back(args[1]);
        }
#ifndef CINDER_COCOA_TOUCH
        mParams = createConfigUI({400, 500});
        ADD_ENUM_TO_INT(mParams.get(), MESH_FILE_ID, mMeshFilenames);
        mParams->addParam("MESH_ROTATION", &mMeshRotation);
        mParams->addButton("Save OBJ", [&] {
            if (!mTriMesh) return;
            fs::path path = mMeshFilenames[mMeshFileId];
            writeObj(writeFile(path.string() + "_new.obj"), mTriMesh);
        });
#endif
        gl::enableDepth();

        ModelGLTF::radianceTexture = am::textureCubeMap(RADIANCE_TEX);
        ModelGLTF::irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
        ModelGLTF::brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);

        getSignalCleanup().connect([&] { writeConfig(); });

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            mMayaCam.setAspectRatio(getWindowAspectRatio());
        });

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
                mTriMesh.reset();
                mVboMesh.reset();
                mModelObj.reset();
                mModelGLTF.reset();

                if (path.extension() == ".obj")
                {
                    if (CI_OBJ_LOADER)
                    {
                        auto leaf = path.filename();
                        auto mtlPath = leaf.generic_string() + "./mtl";
                        if (fs::exists(mtlPath))
                            mTriMesh = am::triMesh(path.string(), mtlPath);
                        else
                            mTriMesh = am::triMesh(path.string());
                        mTriMesh->recalculateNormals();
                        mVboMesh = gl::VboMesh::create(*mTriMesh);
                    }
                    else
                    {
                        mModelObj = ModelObj::create(path, &mLoadingError);
                    }
                }
                else
                {
                    mModelGLTF = ModelGLTF::create(path, &mLoadingError);
                }

                if (!mModelObj && !mVboMesh && !mModelGLTF)
                {
                    mVboMesh = am::vboMesh("Teapot");
                }
            }

            if (FPS_CAMERA)
            {
                CAM_POS_X = mFpsCam.eye.x;
                CAM_POS_Y = mFpsCam.eye.y;
                CAM_POS_Z = mFpsCam.eye.z;
            }
            else
            {
                CAM_POS_X = mMayaCam.getEyePoint().x;
                CAM_POS_Y = mMayaCam.getEyePoint().y;
                CAM_POS_Z = mMayaCam.getEyePoint().z;
            }
            mMayaCam.setNearClip(CAM_Z_NEAR);
            mMayaCam.setFarClip(CAM_Z_FAR);

            if (mModelGLTF)
            {
                mModelGLTF->flipV = FLIP_V;
                mModelGLTF->cameraPosition = mMayaCam.getEyePoint();
                mModelGLTF->update();
            }

            if (mModelObj)
            {
                mModelObj->flipV = FLIP_V;
                mModelObj->cameraPosition = mMayaCam.getEyePoint();
            }

            if (mIsFpsCamera != FPS_CAMERA)
            {
                if (FPS_CAMERA)
                {
                    mFpsCam.setActive(true);
                    mFpsCam.eye = mMayaCam.getEyePoint();
                    //mFpsCam.look = mMayaCam.getPivotPoint();
                }
                else
                {
                    mFpsCam.setActive(false);
                    mMayaCam.lookAt(mFpsCam.eye, mFpsCam.look, mFpsCam.up);
                }
                mIsFpsCamera = FPS_CAMERA;
            }
        });

        getWindow()->getSignalDraw().connect([&] {
            if (mIsFpsCamera)
                gl::setMatrices(mFpsCam);
            else
                gl::setMatrices(mMayaCam);
            gl::clear();

            mParams->show(GUI_VISIBLE);

            if (!mLoadingError.empty())
            {
                texFont->drawString(mLoadingError, { 10, APP_HEIGHT - 150 });
            }

            if (mModelGLTF)
            {
                gl::ScopedTextureBind scpTex(mModelGLTF->radianceTexture, 0);
                if (ENV_VISIBLE)
                {
                    gl::ScopedDepthWrite depthWrite(false);
                    mSkyBoxBatch->draw();
                }

                gl::pointSize(POINT_SIZE);

                gl::setWireframeEnabled(WIRE_FRAME);
                mModelGLTF->currentScene->setScale({ MESH_SCALE,MESH_SCALE,MESH_SCALE });
                mModelGLTF->currentScene->setRotation(mMeshRotation);
                mModelGLTF->draw();
                gl::disableWireframe();
            }

            if (mModelObj)
            {
                gl::setWireframeEnabled(WIRE_FRAME);
                mModelObj->setScale({ MESH_SCALE,MESH_SCALE,MESH_SCALE });
                mModelObj->setRotation(mMeshRotation);
                mModelObj->treeDraw();
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
