#pragma once

#include "../../Cinder-Nodes/include/Node3D.h"
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "syoyo/tiny_gltf.h"

#ifndef CINDER_LESS
#include <cinder/Filesystem.h>
#include <cinder/gl/gl.h>
namespace fs2 = ci::fs;
#else
#include <filesystem>
namespace fs2 = std::experimental::filesystem;

#endif
#include <memory>
#include <vector>

typedef std::shared_ptr<struct RootGLTF> RootGLTFRef;
typedef std::shared_ptr<struct WeakBuffer> WeakBufferRef;

enum AttribGLTF
{
    POSITION,
    COLOR,
    TEX_COORD_0,
    TEX_COORD_1,
    TEX_COORD_2,
    TEX_COORD_3,
    NORMAL,
    TANGENT,
    BITANGENT,
    BONE_INDEX,
    BONE_WEIGHT,
    CUSTOM_0,
    CUSTOM_1,
    CUSTOM_2,
    CUSTOM_3,
    CUSTOM_4,
    CUSTOM_5,
    CUSTOM_6,
    CUSTOM_7,
    CUSTOM_8,
    CUSTOM_9,
    NUM_ATTRIBS,
    USER_DEFINED = NUM_ATTRIBS
};

// WeakBuffer doesn't own memory storage
struct WeakBuffer
{
    WeakBuffer(void* data, size_t size) : mData(data), mDataSize(size) {}
    static WeakBufferRef create(void* buffer, size_t size)
    {
        return std::make_shared<WeakBuffer>(buffer, size);
    }
    size_t getSize() const { return mDataSize; }
    void* getData() { return mData; }
    const void* getData() const { return mData; }

  private:
    void* mData;
    size_t mDataSize;
};

struct AnimationGLTF
{
    typedef std::shared_ptr<AnimationGLTF> Ref;
    tinygltf::Animation property;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Animation& property);
};

struct BufferGLTF
{
    typedef std::shared_ptr<BufferGLTF> Ref;
    tinygltf::Buffer property;
    WeakBufferRef cpuBuffer;
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Buffer& property);
};

struct BufferViewGLTF
{
    typedef std::shared_ptr<BufferViewGLTF> Ref;
    tinygltf::BufferView property;
    WeakBufferRef cpuBuffer; // points to BufferGLTF::cpuBuffer
#ifndef CINDER_LESS
    ci::gl::VboRef gpuBuffer;
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::BufferView& property);
};

struct AccessorGLTF
{
    typedef std::shared_ptr<AccessorGLTF> Ref;
    tinygltf::Accessor property;

    int byteStride;          // from tinygltf::BufferView
    WeakBufferRef cpuBuffer; // points to BufferViewGLTF::cpuBuffer
#ifndef CINDER_LESS
    ci::gl::VboRef gpuBuffer; // points to BufferViewGLTF::gpuBuffer
    // or re-create in case of offseted IBO
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Accessor& property);
};

struct CameraGLTF
{
    typedef std::shared_ptr<CameraGLTF> Ref;
    tinygltf::Camera property;
    tinygltf::PerspectiveCamera perspective;

#ifndef CINDER_LESS
    std::unique_ptr<ci::Camera> camera;
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Camera& property);
};

struct ImageGLTF
{
    typedef std::shared_ptr<ImageGLTF> Ref;
    tinygltf::Image property;

    // BufferViewGLTF::Ref bufferView;
#ifndef CINDER_LESS
    ci::SurfaceRef surface;
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Image& property);
};

struct SamplerGLTF
{
    typedef std::shared_ptr<SamplerGLTF> Ref;
    tinygltf::Sampler property;
#ifndef CINDER_LESS
    ci::gl::SamplerRef ciSampler;
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Sampler& property);
};

struct TextureGLTF
{
    typedef std::shared_ptr<TextureGLTF> Ref;
    tinygltf::Texture property;
#ifndef CINDER_LESS
    ci::gl::Texture2dRef ciTexture;
    ci::gl::SamplerRef ciSampler; // points to SamplerGLTF::ciSampler
    uint8_t textureUnit;
#endif

    void preDraw(uint8_t texUnit = 0);
    void postDraw();

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Texture& property);
};

struct MaterialGLTF
{
    typedef std::shared_ptr<MaterialGLTF> Ref;
    tinygltf::Material property;
    RootGLTFRef rootGLTF;

#ifndef CINDER_LESS
    ci::gl::GlslProgRef ciShader;
#endif

    bool doubleSided = false;

    enum AlphaMode
    {
        OPAQUE,
        MASK,
        BLEND,
    };
    AlphaMode alphaMode = OPAQUE;
    float alphaCutoff = 0.5f;

    TextureGLTF::Ref emissiveTexture;
    glm::vec3 emissiveFactor = {0, 0, 0};

    TextureGLTF::Ref normalTexture;
    int normalTextureCoord = 0;
    float normalTextureScale = 1;

    TextureGLTF::Ref occlusionTexture;
    float occlusionStrength = 1;

    // MetallicRoughness
    TextureGLTF::Ref baseColorTexture;
    glm::vec4 baseColorFacor = {1, 1, 1, 1};
    TextureGLTF::Ref metallicRoughnessTexture;
    float metallicFactor = 1;
    float roughnessFactor = 1;

    // SpecularGlossiness
    TextureGLTF::Ref diffuseTexture;
    int diffuseTextureCoord = 0;
    glm::vec4 diffuseFactor = {1, 1, 1, 1};
    TextureGLTF::Ref specularGlossinessTexture;
    glm::vec3 specularFactor = {1, 1, 1};
    float glossinessFactor = 1;

    enum MaterialType
    {
        MATERIAL_PBR_METAL_ROUGHNESS,
        MATERIAL_PBR_SPEC_GLOSSINESS,
        MATERIAL_UNLIT,

        MATERIAL_COUNT,
    };
    MaterialType materialType;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Material& property);

    void preDraw();

    void postDraw();
};

struct PrimitiveGLTF
{
    typedef std::shared_ptr<PrimitiveGLTF> Ref;
    tinygltf::Primitive property;

    MaterialGLTF::Ref material;

#ifndef CINDER_LESS
    ci::gl::VboMeshRef ciVboMesh;
#endif
    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Primitive& property);

    void update();

    void draw();
};

struct MeshGLTF
{
    typedef std::shared_ptr<MeshGLTF> Ref;
    tinygltf::Mesh property;

    std::vector<PrimitiveGLTF::Ref> primitives;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Mesh& property);

    void update();

    void draw();
};

struct SkinGLTF
{
    typedef std::shared_ptr<SkinGLTF> Ref;
    tinygltf::Skin property;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Skin& property);
};

struct NodeGLTF : public nodes::Node3D
{
    typedef std::shared_ptr<NodeGLTF> Ref;
    tinygltf::Node property;

    CameraGLTF::Ref camera;
    MeshGLTF::Ref mesh;
    SkinGLTF::Ref skin;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Node& property);

    RootGLTFRef rootGLTF;

    void setup();

    void update(double elapsed = 0.0);

    void draw();
};

struct SceneGLTF : public NodeGLTF
{
    typedef std::shared_ptr<SceneGLTF> Ref;
    tinygltf::Scene sceneProperty;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Scene& property);
};

struct RootGLTF
{
    tinygltf::Model property;

    static RootGLTFRef create(const fs2::path& meshPath);

    void update();

    void draw();

    std::vector<AccessorGLTF::Ref> accessors;
    std::vector<AnimationGLTF::Ref> animations;
    std::vector<BufferViewGLTF::Ref> bufferViews;
    std::vector<BufferGLTF::Ref> buffers;
    std::vector<CameraGLTF::Ref> cameras;
    std::vector<ImageGLTF::Ref> images;
    std::vector<MaterialGLTF::Ref> materials;
    std::vector<MeshGLTF::Ref> meshes;
    std::vector<NodeGLTF::Ref> nodes;
    std::vector<SamplerGLTF::Ref> samplers;
    std::vector<SceneGLTF::Ref> scenes;
    std::vector<SkinGLTF::Ref> skins;
    std::vector<TextureGLTF::Ref> textures;

    fs2::path meshPath;

#ifndef CINDER_LESS
    static ci::gl::TextureCubeMapRef radianceTexture;
    static ci::gl::TextureCubeMapRef irradianceTexture;
    static ci::gl::Texture2dRef brdfLUTTexture;
#endif
    bool flipV = true;
    glm::vec3 cameraPosition;

    MaterialGLTF::Ref fallbackMaterial; // if (material == -1)

    SceneGLTF::Ref currentScene;
};
