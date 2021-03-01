#pragma once

#undef near
#undef far
#include "../3rdparty/yocto/yocto_sceneio.h"
#include "../include/Node.h"
#include <filesystem>
#include <Cinder/gl/gl.h>

namespace fs = std::filesystem;

typedef std::shared_ptr<struct GltfScene> GltfSceneRef;

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

#define DebugTypeList \
    ENTRY(DEBUG_NONE, None) \
    ENTRY(DEBUG_BASECOLOR, BaseColor) \
    ENTRY(DEBUG_ALPHA, Alpha) \
    ENTRY(DEBUG_NORMAL, Normal) \
    ENTRY(DEBUG_TANGENT, Tangent) \
    ENTRY(DEBUG_BITANGENT, Bitangent) \
    ENTRY(DEBUG_METALLIC, Metallic) \
    ENTRY(DEBUG_ROUGHNESS, Roughness) \
    ENTRY(DEBUG_OCCLUSION, Occulussion) \
    ENTRY(DEBUG_F0, F0) \
    ENTRY(DEBUG_FEMISSIVE, Emissive) \
    ENTRY(DEBUG_FSPECULAR, Specular) \
    ENTRY(DEBUG_FDIFFUSE, Diffuse) \
    ENTRY(DEBUG_FSHEEN, Sheen) \
    ENTRY(DEBUG_FCLEARCOAT, ClearCoat) \
    ENTRY(DEBUG_FSUBSURFACE, Subsurface) \
    ENTRY(DEBUG_THICKNESS, Thickness) \
    ENTRY(DEBUG_FTRANSMISSION, Transmission)

#define ENTRY(flag, name) flag,
enum DebugType
{
    DebugTypeList

    DEBUG_COUNT,
};
#undef ENTRY

struct GltfLight
{
    glm::vec3 direction = { 0,1.0f, 0 };
    float range = { 999 };

    glm::vec3 color = { 1, 1, 1 };
    float intensity = 10;

    glm::vec3 position;
    float innerConeCos = 0.1;

    float outerConeCos = 0.5;
    int type = LightType_Directional;

    glm::vec2 padding;
};

struct GltfMaterial
{
    typedef std::shared_ptr<GltfMaterial> Ref;
    static Ref create(GltfScene* scene, yocto::scene_material& property, DebugType debugType = DEBUG_NONE);

    yocto::scene_material property;

    ci::gl::Texture2dRef emission_tex;
    ci::gl::Texture2dRef color_tex;
    ci::gl::Texture2dRef roughness_tex;
    ci::gl::Texture2dRef scattering_tex;
    ci::gl::Texture2dRef normal_tex;
    ci::gl::Texture2dRef occulusion_tex;

    void bind();

    void unbind();

    ci::gl::GlslProgRef glsl;
};

struct GltfNode : melo::Node
{
    typedef std::shared_ptr<GltfNode> Ref;
    static Ref create(GltfScene* scene, yocto::scene_instance& property);

    ci::gl::VboMeshRef mesh;
    GltfMaterial::Ref material;

    void draw(melo::DrawOrder order) override;
    void reloadMaterial();

    GltfScene* scene;
    yocto::scene_instance property;
};

struct GltfScene : melo::Node
{
    static void progress_callback(const std::string& message, int current, int total);

    static GltfSceneRef create(const fs::path& path);

    fs::path path;

    GltfLight lights[1] = {};
    std::vector<ci::gl::VboMeshRef> meshes;
    std::vector<ci::gl::Texture2dRef> textures;
    std::vector<GltfMaterial::Ref> materials;

    yocto::scene_scene property;

    static ci::gl::TextureCubeMapRef radianceTexture;
    static ci::gl::TextureCubeMapRef irradianceTexture;
    static ci::gl::Texture2dRef brdfLUTTexture;

    void createMaterials(DebugType debugType = DEBUG_NONE);

    ci::gl::Texture2dRef getTexture(yocto::texture_handle handle)
    {
        if (handle == yocto::invalid_handle) return {};
        return textures[handle];
    }

    ci::gl::VboMeshRef getMesh(yocto::shape_handle handle)
    {
        if (handle == yocto::invalid_handle) return {};
        return meshes[handle];
    }

    GltfMaterial::Ref getMaterial(yocto::material_handle handle)
    {
        if (handle == yocto::invalid_handle) return {};
        return materials[handle];
    }

    void update(double elapsed) override;

    void predraw(melo::DrawOrder order) override;

    void postdraw(melo::DrawOrder order) override;

    bool isMaterialDirty = false;

private:

    ci::gl::Texture2dRef createTexture(const yocto::scene_texture& texture);

    ci::gl::VboMeshRef createMesh(const yocto::scene_shape& shape);
};