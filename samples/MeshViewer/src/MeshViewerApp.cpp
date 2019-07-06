#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ObjLoader.h"
#include "cinder/params/Params.h"
#include "cinder/ip/Flip.h"
#include "AssetManager.h"
#include "MiniConfig.h"
#include "FontHelper.h"

#include "melo.h"
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

Surface copyWindowSurfaceWithAlpha() {
    Area area = getWindowBounds();
    Surface s(area.getWidth(), area.getHeight(), true);
    glFlush();
    GLint oldPackAlignment;
    glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(area.x1, getWindowHeight() - area.y2, area.getWidth(), area.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, s.getData());
    glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
    ip::flipVertical(&s);
    return s;
}

struct MeloViewer : public App
{
    CameraPersp mMayaCam;
    CameraUi mMayaCamUi;
    Camera* mCurrentCam = nullptr;
    bool mIsFpsCamera = false;
    quat mMeshRotation;

    // args
    bool mSnapshotMode = false;
    string mOutputFilename;

    gl::GlslProgRef mGlslProg;
    gl::BatchRef mSkyBoxBatch;
    int mMeshFileId = -1;
    vector<string> mMeshFilenames;

    // 1 of 3
    ModelGLTFRef mModelGLTF;
    ModelObjRef mModelObj;
    gl::VboMeshRef mVboMesh;
    TriMeshRef mTriMesh;

    nodes::Node3DRef mGridNode;

    FirstPersonCamera mFpsCam;

    string mLoadingError;
    gl::TextureFontRef texFont;
    params::InterfaceGlRef mParams;

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
        log::makeLogger<log::LoggerFile>();
        addAssetDirectory(getAppPath() / "../assets");
        addAssetDirectory(getAppPath() / "../../../assets");

        mMayaCam.lookAt({ CAM_POS_X, CAM_POS_Y, CAM_POS_Z }, vec3(), vec3(0, 1, 0));
        mMayaCamUi = CameraUi(&mMayaCam, getWindow(), -1);
        mFpsCam.setup();

        texFont = FontHelper::createTextureFont("Helvetica", 24);

        parseArgs();

        if (!mSnapshotMode)
            mMeshFilenames = listGlTFFiles();

#ifndef CINDER_COCOA_TOUCH
        mParams = createConfigUI({ 400, 500 });
        ADD_ENUM_TO_INT(mParams.get(), MESH_FILE_ID, mMeshFilenames);
        mParams->addParam("MESH_ROTATION", &mMeshRotation);
        mParams->addButton("Save OBJ", [&] {
            if (!mTriMesh) return;
            fs::path path = mMeshFilenames[mMeshFileId];
            writeObj(writeFile(path.string() + "_new.obj"), mTriMesh);
            });
#endif
        gl::enableDepth();

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

                bool isImageType = std::find(imageExts.begin(), imageExts.end(), filePath.extension()) != imageExts.end();
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
                        auto box = mModelObj->boundingBox;
                        if (true || mSnapshotMode)
                            // TODO: ugly
                            mMayaCam.lookAt(box.getMax() * vec3(CAM_NEW_MESH_DISTANCE_X, CAM_NEW_MESH_DISTANCE_Y, CAM_NEW_MESH_DISTANCE_Z), box.getCenter());
                        else
                            mMayaCam.lookAt(box.getMax() * vec3(0, 0, 1), box.getCenter());
                    }
                }
                else
                {
                    if (ModelGLTF::radianceTexture == nullptr)
                    {
                        ModelGLTF::radianceTexture = am::textureCubeMap(RADIANCE_TEX);
                        ModelGLTF::irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
                        ModelGLTF::brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);
                    }

                    mModelGLTF = ModelGLTF::create(path, &mLoadingError);
                    auto box = mModelGLTF->boundingBox;
                    if (true || mSnapshotMode)
                        // TODO: ugly*2
                        mMayaCam.lookAt(box.getMax() * vec3(CAM_NEW_MESH_DISTANCE_X, CAM_NEW_MESH_DISTANCE_Y, CAM_NEW_MESH_DISTANCE_Z), box.getCenter());
                    else
                        mMayaCam.lookAt(box.getMax() * vec3(0, 0, 1), box.getCenter());
                }

                if (!mModelObj && !mVboMesh && !mModelGLTF)
                {
                    mVboMesh = am::vboMesh("Teapot");
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

            if (mModelGLTF)
            {
                mModelGLTF->flipV = FLIP_V;
                mModelGLTF->cameraPosition = mCurrentCam->getEyePoint();
                mModelGLTF->treeUpdate();
            }

            if (mModelObj)
            {
                mModelObj->flipV = FLIP_V;
                mModelObj->cameraPosition = mCurrentCam->getEyePoint();
            }
            });

        getWindow()->getSignalDraw().connect([&] {
            if (mIsFpsCamera)
                gl::setMatrices(mFpsCam);
            else
                gl::setMatrices(mMayaCam);
            if (mSnapshotMode)
                gl::clear(ColorA::gray(0, 0.0f));
            else
                gl::clear(Color::gray(0.2f));


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
                    if (mSkyBoxBatch == nullptr)
                    {
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
                    }
                    mSkyBoxBatch->draw();
                }

                gl::pointSize(POINT_SIZE);

                gl::setWireframeEnabled(WIRE_FRAME);
                mModelGLTF->setScale({ MESH_SCALE,MESH_SCALE,MESH_SCALE });
                mModelGLTF->setRotation(mMeshRotation);
                mModelGLTF->treeDraw();
                gl::disableWireframe();
            }

            if (mModelObj)
            {
                gl::setWireframeEnabled(WIRE_FRAME);
                gl::ScopedTextureBind tex0(am::texture2d(TEX0_NAME));
                mModelObj->setScale({ MESH_SCALE,MESH_SCALE,MESH_SCALE });
                mModelObj->setRotation(mMeshRotation);
                mModelObj->treeDraw();
                gl::disableWireframe();
            }

            if (mVboMesh)
            {
                gl::ScopedGlslProg glsl(am::glslProg("lambert texture"));
                gl::ScopedTextureBind tex0(am::texture2d(TEX0_NAME));
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
                if (mGridNode == nullptr)
                    mGridNode = nodes::GridNode::create();
                mGridNode->treeDraw();
                gl::ScopedDepthTest depthTest(false);
                gl::drawCoordinateFrame(10, 0.5, 0.1);
            }

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
