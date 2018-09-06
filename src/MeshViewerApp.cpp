#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include "Cinder-Nodes/include/Node3D.h"

#include "syoyo/tiny_gltf.h"
//#include "syoyo/tiny_obj_loader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace cigltf
{
    struct Modal;

    typedef std::shared_ptr<struct Node> NodeRef;
    struct Node : public nodes::Node3D
    {
        int mesh;
        void draw() {}
    };

    struct Scene;
    struct Node;
    struct Mesh
    {
        vector<gl::VboMeshRef> primitive;
    };
    struct Camera;
    struct Light;

    struct Accessor;
    struct Buffer;
    struct BufferView;

    struct Image;
    struct Texture;
    struct Sampler;
    struct Material;

    struct Skin;
    struct Animation;

    struct Model
    {
        tinygltf::Model tiny_model;
        vector<NodeRef> nodes;
        vector<shared_ptr<Camera>> cameras;
        vector<gl::Texture2dRef> textures; // texture + image
        vector<gl::SamplerRef> samplers;
        vector<gl::BufferObjRef> bufferViews;
        vector<Mesh> meshes;
    };
} // namespace cigltf

namespace cigltf
{
    shared_ptr<Modal> gltfMesh(const string& filename)
    {
        fs::path fullPath = getAssetPath(filename);

        tinygltf::TinyGLTF loader;
        tinygltf::Model tiny_model;
        std::string err;
        std::string warn;
        std::string input_filename(fullPath.string());
        std::string ext = fullPath.extension().string();

        bool ret = false;
        if (ext.compare(".glb") == 0)
        {
            // assume binary glTF.
            ret = loader.LoadBinaryFromFile(&tiny_model, &err, &warn, input_filename.c_str());
        }
        else
        {
            // assume ascii glTF.
            ret = loader.LoadASCIIFromFile(&tiny_model, &err, &warn, input_filename.c_str());
        }

        if (!warn.empty())
        {
            CI_LOG_W(warn);
        }

        if (!err.empty())
        {
            CI_LOG_E(err);
        }
        if (!ret)
        {
            CI_LOG_F("Failed to load .glTF") << fullPath;
            App::get()->quit();
        }

        shared_ptr<Modal> ciModal = make_shared<Modal>();
        ciModal->tiny_model = tiny_model;

        for (auto& it : model.bufferViews)
        {
            auto pData = model.buffers[it.buffer].data.data();
            auto obj = gl::BufferObj::create(it.target, byteLength, pData, GL_DYNAMIC_DRAW);
            ciModal.bufferViews.emplace_back(obj);
        }

        for (auto& it : model.samplers)
        {
            auto fmt = gl::Sampler::Format()
                           .setWrap(it.warpS, it.warpT, it.wrapR)
                           .minFilter(it.minFilter)
                           .maxFilter(it.maxFilter);
            auto obj = gl::Sampler::create(fmt);
            ciModal.samplers.emplace_back(obj);
        }

        for (auto& it : model.textures)
        {
            Image& source = model.images[it.source];
            auto obj = gl::Texture2d::create()
        }

        return ciModal;
    }
} // namespace cigltf

class MeshViewerApp : public App
{
  public:
    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        auto aabb = cigltf::gltfMesh(MESH_NAME)->calcBoundingBox();
        mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());
        mCamUi = CameraUi(&mCam, getWindow(), -1);

        createConfigUI({200, 200});
        gl::enableDepth();

        getWindow()->getSignalResize().connect(
            [&] { mCam.setAspectRatio(getWindowAspectRatio()); });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        mGlslProg = am::glslProg(VS_NAME, FS_NAME);
        mGlslProg->uniform("uTex0", 0);
        mGlslProg->uniform("uTex1", 1);
        mGlslProg->uniform("uTex2", 2);
        mGlslProg->uniform("uTex3", 3);

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatrices(mCam);
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
    CameraPersp mCam;
    CameraUi mCamUi;
    gl::GlslProgRef mGlslProg;
};

CINDER_APP(MeshViewerApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
