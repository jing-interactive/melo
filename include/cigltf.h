#pragma once

#ifdef CINDER_LESS
#include <filesystem>
namespace fs2 = std::filesystem;
#else
#include <cinder/Filesystem.h>
#include <cinder/gl/gl.h>
#include <cinder/Timeline.h>
namespace fs2 = ci::fs;
#endif

#include <memory>
#include <vector>

#include "../3rdparty/tinygltf/tiny_gltf.h"

#include "Node.h"

typedef std::shared_ptr<struct ModelGLTF> ModelGLTFRef;
typedef std::shared_ptr<struct WeakBuffer> WeakBufferRef;

enum GltfMode
{
    MODE_POINTS = 0,
    MODE_LINE = 1,
    MODE_LINE_LOOP = 2,
    MODE_TRIANGLES = 4,
    MODE_TRIANGLE_STRIP = 5,
    MODE_TRIANGLE_FAN = 6,
};

enum GltfType
{
    TYPE_UNDEFINED = 0,
    TYPE_VEC2 = 2,
    TYPE_VEC3 = 3,
    TYPE_VEC4 = 4,
    TYPE_MAT2 = 32 + 2,
    TYPE_MAT3 = 32 + 3,
    TYPE_MAT4 = 32 + 4,
    TYPE_SCALAR = 64 + 1,
    TYPE_VECTOR = 64 + 4,
    TYPE_MATRIX = 64 + 16,
};

enum GltfComponentType
{
    COMPONENT_TYPE_UNDEFINED = 0,
    COMPONENT_TYPE_BYTE = 5120,
    COMPONENT_TYPE_UNSIGNED_BYTE = 5121,
    COMPONENT_TYPE_SHORT = 5122,
    COMPONENT_TYPE_UNSIGNED_SHORT = 5123,
    COMPONENT_TYPE_INT = 5124,
    COMPONENT_TYPE_UNSIGNED_INT = 5125,
    COMPONENT_TYPE_FLOAT = 5126,
    COMPONENT_TYPE_DOUBLE = 5130,
};

enum GltfTarget
{
    TARGET_ARRAY_BUFFER = 34962,
    TARGET_ELEMENT_ARRAY_BUFFER = 34963,
};

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

    NUM_ATTRIBS,
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

    GltfType type = TYPE_UNDEFINED;
    GltfComponentType componentType = COMPONENT_TYPE_UNDEFINED;

  private:
    void* mData;
    size_t mDataSize;
};

struct AnimationChannel
{
    tinygltf::AnimationChannel property;

    enum PathType { TRANSLATION, ROTATION, SCALE };
    PathType path;
    int node;
    uint32_t samplerIndex;
};

struct AnimationSampler
{
    tinygltf::AnimationSampler property;

    enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;

    ci::Anim<glm::vec4> value;
    void apply();
};

struct AnimationGLTF
{
    typedef std::shared_ptr<AnimationGLTF> Ref;
    tinygltf::Animation property;

    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();

    void apply();
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Animation& property);
};

struct BufferGLTF
{
    typedef std::shared_ptr<BufferGLTF> Ref;
    tinygltf::Buffer property;
    WeakBufferRef cpuBuffer;
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Buffer& property);
};

struct BufferViewGLTF
{
    typedef std::shared_ptr<BufferViewGLTF> Ref;
    tinygltf::BufferView property;
    GltfTarget target;
    WeakBufferRef cpuBuffer; // points to BufferGLTF::cpuBuffer + offset
#ifndef CINDER_LESS
    ci::gl::VboRef gpuBuffer;
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::BufferView& property);
};

struct AccessorGLTF
{
    typedef std::shared_ptr<AccessorGLTF> Ref;
    tinygltf::Accessor property;

    int byteStride;          // from tinygltf::BufferView
    WeakBufferRef cpuBuffer; // points to BufferViewGLTF::cpuBuffer + offset
#ifndef CINDER_LESS
    ci::gl::VboRef gpuBuffer; // points to BufferViewGLTF::gpuBuffer
    // or re-create in case of offseted IBO
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Accessor& property);
};

struct CameraGLTF
{
    typedef std::shared_ptr<CameraGLTF> Ref;
    tinygltf::Camera property;
    tinygltf::PerspectiveCamera perspective;

#ifndef CINDER_LESS
    std::unique_ptr<ci::Camera> camera;
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Camera& property);
};

struct ImageGLTF
{
    typedef std::shared_ptr<ImageGLTF> Ref;
    tinygltf::Image property;

    // BufferViewGLTF::Ref bufferView;
#ifndef CINDER_LESS
    ci::SurfaceRef surface;
    ci::DataSourceRef compressedSurface;
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Image& property);
};

struct SamplerGLTF
{
    typedef std::shared_ptr<SamplerGLTF> Ref;
    tinygltf::Sampler property;
#ifndef CINDER_LESS
    ci::gl::SamplerRef ciSampler;
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Sampler& property);
};

struct TextureGLTF
{
    typedef std::shared_ptr<TextureGLTF> Ref;
    tinygltf::Texture property;
    ImageGLTF::Ref imageSource;
#ifndef CINDER_LESS
    ci::gl::Texture2dRef ciTexture;
    ci::gl::SamplerRef ciSampler; // points to SamplerGLTF::ciSampler
    uint8_t textureUnit;
#endif

    void predraw(uint8_t texUnit = 0);
    void postdraw();

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Texture& property);
};

struct MaterialGLTF
{
    typedef std::shared_ptr<MaterialGLTF> Ref;
    tinygltf::Material property;
    ModelGLTFRef modelGLTF;

#ifndef CINDER_LESS
    ci::gl::GlslProg::Format ciShaderFormat;
    ci::gl::GlslProgRef ciShader; // creation of ciShader will be deferred
#endif

    bool doubleSided = false;

    enum AlphaMode
    {
        ALPHA_OPAQUE,
        ALPHA_MASK,
        ALPHA_BLEND,
    };
    AlphaMode alphaMode = ALPHA_OPAQUE;
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

    melo::MaterialType materialType;

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Material& property);

    void predraw();

    void postdraw();
};

struct PrimitiveGLTF
{
    typedef std::shared_ptr<PrimitiveGLTF> Ref;
    tinygltf::Primitive property;
    GltfMode primitiveMode;

    MaterialGLTF::Ref material;

    WeakBufferRef indices;   // uint32_t[]
    uint32_t indexCount;
    uint32_t vertexCount;
    WeakBufferRef positions; // vec3[]
    WeakBufferRef normals;   // vec3[]
    //WeakBufferRef tangents;  // vec4[]
    WeakBufferRef uvs;       // vec2[]

#ifndef CINDER_LESS
    ci::gl::VboMeshRef ciVboMesh;
#endif
    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Primitive& property);

    void update();

    void draw();
};

struct MeshGLTF
{
    typedef std::shared_ptr<MeshGLTF> Ref;
    tinygltf::Mesh property;

    std::vector<PrimitiveGLTF::Ref> primitives;

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Mesh& property);

    void update();

    void draw();
};

struct SkinGLTF
{
    typedef std::shared_ptr<SkinGLTF> Ref;
    tinygltf::Skin property;

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Skin& property);
};

struct NodeGLTF : public melo::Node
{
    typedef std::shared_ptr<NodeGLTF> Ref;
    tinygltf::Node property;

    CameraGLTF::Ref camera;
    MeshGLTF::Ref mesh;
    SkinGLTF::Ref skin;

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Node& property);

    ModelGLTFRef modelGLTF;

    void setup();

    void update(double elapsed = 0.0);

    void predraw();
    void draw();
    void postdraw();
};

struct SceneGLTF : public NodeGLTF
{
    typedef std::shared_ptr<SceneGLTF> Ref;
    tinygltf::Scene sceneProperty;

    static Ref create(ModelGLTFRef modelGLTF, const tinygltf::Scene& property);
};

struct ModelGLTF : public melo::Node
{
    tinygltf::Model property;

    static ModelGLTFRef create(const fs2::path& meshPath, std::string* loadingError = nullptr);

    //void update(double elapsed = 0.0) override;

    void predraw() override;
    void postdraw() override;

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

    bool flipV = true;

    MaterialGLTF::Ref fallbackMaterial; // if (material == -1)

    SceneGLTF::Ref currentScene;
};
