#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include "syoyo/tiny_gltf.h"
#include "syoyo/tiny_obj_loader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace am
{
    TriMeshRef gltfMesh(const string& filename)
    {
        fs::path fullPath = getAssetPath(filename);
        TriMeshRef mesh;
        
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        std::string input_filename(fullPath.string());
        std::string ext = fullPath.extension().string();
        
        bool ret = false;
        if (ext.compare(".glb") == 0) {
            // assume binary glTF.
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
        } else {
            // assume ascii glTF.
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
        }
        
        if (!warn.empty()) {
            CI_LOG_W(warn);
        }
        
        if (!err.empty()) {
            CI_LOG_E(err);
        }
        if (!ret) {
            CI_LOG_F("Failed to load .glTF") << fullPath;
            App::get()->quit();
        }
        
        return mesh;
    }
}
class MeshViewerApp : public App
{
  public:
    void setup() override
    {
        log::makeLogger<log::LoggerFile>();
        
        auto aabb = am::gltfMesh(MESH_NAME)->calcBoundingBox();
        mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());
        mCamUi = CameraUi( &mCam, getWindow(), -1 );
        
        createConfigUI({200, 200});
        gl::enableDepth();
    
        getWindow()->getSignalResize().connect([&] {
            mCam.setAspectRatio( getWindowAspectRatio() );
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });
        
        mGlslProg = am::glslProg(VS_NAME, FS_NAME);
        mGlslProg->uniform("uTex0", 0);
        mGlslProg->uniform("uTex1", 1);
        mGlslProg->uniform("uTex2", 2);
        mGlslProg->uniform("uTex3", 3);

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices( mCam );
            gl::clear();
        
            gl::ScopedTextureBind tex0(am::texture2d(TEX0_NAME), 0);
            gl::ScopedTextureBind tex1(am::texture2d(TEX1_NAME), 1);
            gl::ScopedTextureBind tex2(am::texture2d(TEX2_NAME), 2);
            gl::ScopedTextureBind tex3(am::texture2d(TEX3_NAME), 3);
            gl::ScopedGlslProg glsl(mGlslProg);

            gl::draw(am::vboMesh(MESH_NAME));
        });
    }
    
private:
    CameraPersp         mCam;
    CameraUi            mCamUi;
    gl::GlslProgRef     mGlslProg;
};

CINDER_APP( MeshViewerApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
} )
