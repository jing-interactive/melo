#pragma once

#include "Cinder-Nodes/include/Node3D.h"
#include "syoyo/tiny_gltf.h"
#include <cinder/gl/gl.h>
#include <memory>
#include <vector>

typedef std::shared_ptr<struct RootGLTF> RootGLTFRef;

using namespace ci;

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

    BufferRef cpuBuffer;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Buffer& property);
};

struct BufferViewGLTF
{
    typedef std::shared_ptr<BufferViewGLTF> Ref;
    tinygltf::BufferView property;

    BufferRef cpuBuffer;
    gl::VboRef gpuBuffer;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::BufferView& property);
};

struct AccessorGLTF
{
    typedef std::shared_ptr<AccessorGLTF> Ref;
    tinygltf::Accessor property;
    int byteStride;       // from tinygltf::BufferView
    gl::VboRef gpuBuffer; // points to BufferViewGLTF::gpuBuffer

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Accessor& property);
};

struct CameraGLTF
{
    typedef std::shared_ptr<CameraGLTF> Ref;
    tinygltf::Camera property;

    std::unique_ptr<Camera> camera;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Camera& property);
};

struct ImageGLTF
{
    typedef std::shared_ptr<ImageGLTF> Ref;
    tinygltf::Image property;

    // BufferViewGLTF::Ref bufferView;
    SurfaceRef surface;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Image& property);
};

struct SamplerGLTF
{
    typedef std::shared_ptr<SamplerGLTF> Ref;
    tinygltf::Sampler property;

    gl::Sampler::Format ciFormat;

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Sampler& property);
};

struct TextureGLTF
{
    typedef std::shared_ptr<TextureGLTF> Ref;
    tinygltf::Texture property;

    gl::Texture2dRef ciTexture;
    gl::SamplerRef ciSampler;
    uint8_t textureUnit;

    void preDraw(uint8_t texUnit = 0);
    void postDraw();

    static Ref create(RootGLTFRef rootGLTF, const tinygltf::Texture& property);
};

struct MaterialGLTF
{
    typedef std::shared_ptr<MaterialGLTF> Ref;
    tinygltf::Material property;
    RootGLTFRef rootGLTF;

    gl::GlslProgRef ciShader;

    bool doubleSided = false;

    TextureGLTF::Ref emissiveTexture;
    vec3 emissiveFactor = {0, 0, 0};

    TextureGLTF::Ref normalTexture;
    int normalTextureCoord = 0;
    float normalTextureScale = 1;

    TextureGLTF::Ref occlusionTexture;
    float occlusionStrength = 1;

    // MetallicRoughness
    TextureGLTF::Ref baseColorTexture;
    vec4 baseColorFacor = {1, 1, 1, 1};
    TextureGLTF::Ref metallicRoughnessTexture;
    float metallicFactor = 1;
    float roughnessFactor = 1;

    // SpecularGlossiness
    TextureGLTF::Ref diffuseTexture;
    int diffuseTextureCoord = 0;
    vec4 diffuseFactor = {1, 1, 1, 1};
    TextureGLTF::Ref specularGlossinessTexture;
    vec3 specularFactor = {1, 1, 1};
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

    gl::VboMeshRef ciVboMesh;

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

    void update();

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

    static RootGLTFRef create(const fs::path& gltfPath);

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

    fs::path gltfPath;

    gl::TextureCubeMapRef radianceTexture;
    gl::TextureCubeMapRef irradianceTexture;
    gl::Texture2dRef brdfLUTTexture;
    bool flipV = true;
    vec3 cameraPosition;

    SceneGLTF::Ref currentScene;
};
