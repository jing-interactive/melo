#include "../include/GltfNode.h"
#include <Cinder/app/App.h>
#include <Cinder/Log.h>
#include "CinderRemotery.h"

using namespace ci;
using namespace app;
using namespace std;

gl::TextureCubeMapRef GltfScene::radianceTexture;
gl::TextureCubeMapRef GltfScene::irradianceTexture;
gl::Texture2dRef GltfScene::brdfLUTTexture;

void GltfScene::predraw(melo::DrawOrder order)
{
    auto folderPath = path.parent_path().filename();
    auto folderName = folderPath.string();
    rmt_BeginCPUSampleDynamic(folderName.c_str(), 0);
    rmt_BeginOpenGLSampleDynamic(folderName.c_str());
    if (GltfScene::brdfLUTTexture && GltfScene::irradianceTexture && GltfScene::radianceTexture)
    {
        GltfScene::radianceTexture->bind(7);
        GltfScene::irradianceTexture->bind(8);
        GltfScene::brdfLUTTexture->bind(9);
    }
}

void GltfScene::postdraw(melo::DrawOrder order)
{
    rmt_EndCPUSample();
    rmt_EndOpenGLSample();
}

GltfMaterial::Ref GltfMaterial::create(GltfScene* scene, yocto::scene_material& property, DebugType debugType)
{
    auto ref = make_shared<GltfMaterial>();
    ref->property = property;
    ref->color_tex = scene->getTexture(property.color_tex);
    ref->normal_tex = scene->getTexture(property.normal_tex);
    ref->roughness_tex = scene->getTexture(property.roughness_tex);
    ref->emission_tex = scene->getTexture(property.emission_tex);
    ref->scattering_tex = scene->getTexture(property.scattering_tex);
    ref->occulusion_tex = scene->getTexture(property.occulusion_tex);

    auto fmt = gl::GlslProg::Format();
    fmt.define("HAS_NORMALS");
    //fmt.define("HAS_TANGENTS");
    fmt.define("HAS_UV_SET1");
    fmt.define("USE_PUNCTUAL");
    fmt.define("LIGHT_COUNT", "1");
    if (property.type == yocto::material_type::metallic)
    {
        fmt.define("MATERIAL_METALLICROUGHNESS");
        if (GltfScene::brdfLUTTexture && GltfScene::irradianceTexture && GltfScene::radianceTexture)
        {
            fmt.define("USE_IBL");
        }
    }
    else if (property.type == yocto::material_type::subsurface)
    {
        fmt.define("MATERIAL_METALLICROUGHNESS");
        fmt.define("MATERIAL_SUBSURFACE");
        if (ref->scattering_tex)
            fmt.define("HAS_SUBSURFACE_THICKNESS_MAP"); // TODO: HAS_SUBSURFACE_COLOR_MAP
    }
    else if (property.type == yocto::material_type::metal)
    {
        fmt.define("MATERIAL_METALLICROUGHNESS");
        fmt.define("MATERIAL_CLEARCOAT");
        if (ref->scattering_tex)
            fmt.define("HAS_CLEARCOAT_ROUGHNESS_MAP");
    }
    else if (property.type == yocto::material_type::leaves)
    {
        fmt.define("MATERIAL_METALLICROUGHNESS");
        fmt.define("MATERIAL_SHEEN");
        if (ref->scattering_tex)
            fmt.define("HAS_SHEEN_COLOR_INTENSITY_MAP");
    }
    else
        fmt.define("MATERIAL_UNLIT");

    if (ref->color_tex)
        fmt.define("HAS_BASE_COLOR_MAP");
    if (ref->roughness_tex)
        fmt.define("HAS_METALLIC_ROUGHNESS_MAP");
    if (ref->emission_tex)
        fmt.define("HAS_EMISSIVE_MAP");
    if (ref->normal_tex)
        fmt.define("HAS_NORMAL_MAP");
    if (ref->occulusion_tex)
        fmt.define("HAS_OCCLUSION_MAP");

    if (property.opacity > 0)
        fmt.define("ALPHAMODE_OPAQUE");
    else if (property.opacity < 0)
        fmt.define("ALPHAMODE_MASK");

    if (debugType != DEBUG_NONE)
    {
        fmt.define("DEBUG_OUTPUT");

#define ENTRY(flag, name) if (debugType == flag) fmt.define(#flag);
        DebugTypeList
#undef ENTRY
    }

    fmt.attrib(geom::POSITION, "a_Position");
    fmt.attrib(geom::NORMAL, "a_Normal");
    fmt.attrib(geom::TANGENT, "a_Tangent");
    fmt.attrib(geom::TEX_COORD_0, "a_UV1");
    fmt.attrib(geom::TEX_COORD_1, "a_UV2");
    fmt.attrib(geom::COLOR, "a_Color");

    fmt.uniform(gl::UNIFORM_VIEW_PROJECTION, "u_ViewProjectionMatrix");
    fmt.uniform(gl::UNIFORM_MODEL_MATRIX, "u_ModelMatrix");
    fmt.uniform(gl::UNIFORM_NORMAL_MATRIX, "u_NormalMatrix");

    fmt.fragDataLocation(0, "g_finalColor");

    fmt.vertex(DataSourcePath::create(app::getAssetPath("pbr/primitive.vert")));
    fmt.fragment(DataSourcePath::create(app::getAssetPath("pbr/pbr.frag")));
    fmt.label("khronos-pbr");

    try
    {
#if 1
        ref->glsl = gl::GlslProg::create(fmt);
#else
        ref->glsl = am::glslProg("lambert texture");
#endif
        if (ref->color_tex)
            ref->glsl->uniform("u_BaseColorSampler", 0);
        if (ref->normal_tex)
            ref->glsl->uniform("u_NormalSampler", 1);
        if (ref->emission_tex)
            ref->glsl->uniform("u_EmissiveSampler", 2);
        if (ref->roughness_tex)
            ref->glsl->uniform("u_MetallicRoughnessSampler", 3);
        if (ref->occulusion_tex)
            ref->glsl->uniform("u_OcclusionSampler", 4);

        if (GltfScene::brdfLUTTexture && GltfScene::irradianceTexture && GltfScene::radianceTexture)
        {
            ref->glsl->uniform("u_LambertianEnvSampler", 7);
            ref->glsl->uniform("u_GGXEnvSampler", 8);
            ref->glsl->uniform("u_GGXLUT", 9);
        }
    }
    catch (Exception& e)
    {
        CI_LOG_E("Create shader failed, reason: \n" << e.what());
    }

    return ref;
}

void GltfMaterial::bind()
{
    glsl->uniform("u_MetallicFactor", property.metallic);
    glsl->uniform("u_RoughnessFactor", property.roughness);
    glsl->uniform("u_BaseColorFactor", glm::vec4{ property.color.x, property.color.y, property.color.z, 1.0f });
    glsl->uniform("u_NormalScale", 1.0f);
    glsl->uniform("u_EmissiveFactor", (glm::vec3&)property.emission);
    if (occulusion_tex)
        glsl->uniform("u_OcclusionStrength", property.occulusion_strength);

    if (property.type == yocto::material_type::subsurface)
    {
        // TODO: use MATERIAL_VOLUME / HAS_THICKNESS_MAP
        glsl->uniform("u_SubsurfaceScale", 5.0f);
        glsl->uniform("u_SubsurfaceDistortion", 0.2f);
        glsl->uniform("u_SubsurfacePower", 4.0f);
        glsl->uniform("u_SubsurfaceColorFactor", (glm::vec3&)property.scattering);
        glsl->uniform("u_SubsurfaceThicknessFactor", 1.0f);
        glsl->uniform("u_SubsurfaceThicknessSampler", 5);
    }
    else if (property.type == yocto::material_type::metal)
    {
        glsl->uniform("u_ClearcoatFactor", property.scanisotropy);
        glsl->uniform("u_ClearcoatRoughnessFactor", property.ior);
        glsl->uniform("u_ClearcoatRoughnessSampler", 5);
    }
    else if (property.type == yocto::material_type::leaves)
    {
        glsl->uniform("u_SheenIntensityFactor", 1.0f);
        glsl->uniform("u_SheenColorFactor", (glm::vec3&)property.scattering);
        glsl->uniform("u_SheenRoughness", property.ior);
        glsl->uniform("u_SheenColorIntensitySampler", 5);
    }

    if (property.opacity < 0)
    {
        auto alphaCutoff = -property.opacity;
        glsl->uniform("u_AlphaCutoff", alphaCutoff);
    }

    if (glsl)
        glsl->bind();
    if (color_tex)
        color_tex->bind(0);
    if (normal_tex)
        normal_tex->bind(1);
    if (emission_tex)
        emission_tex->bind(2);
    if (roughness_tex)
        roughness_tex->bind(3);
    if (occulusion_tex)
        occulusion_tex->bind(4);
    if (scattering_tex)
        scattering_tex->bind(5);
}

void GltfMaterial::unbind()
{
    if (color_tex)
        color_tex->unbind();
    if (normal_tex)
        normal_tex->unbind();
    if (emission_tex)
        emission_tex->unbind();
    if (roughness_tex)
        roughness_tex->unbind();
    if (occulusion_tex)
        occulusion_tex->unbind();
    if (scattering_tex)
        scattering_tex->unbind();
}

void GltfNode::reloadMaterial()
{
    if (property.material != yocto::invalid_handle)
    {
        material = scene->getMaterial(property.material);
        if (material->property.opacity == 0)
            mDrawOrder = melo::DRAW_TRANSPARENCY;
    }
}

GltfNode::Ref GltfNode::create(GltfScene* scene, yocto::scene_instance& property)
{
    auto ref = make_shared<GltfNode>();

    ref->scene = scene;
    ref->property = property;
    auto transform = yocto::frame_to_mat(property.frame);
    ref->setConstantTransform(glm::make_mat4((const float*)&transform.x));

    ref->mesh = scene->getMesh(property.shape);

    ref->reloadMaterial();

    return ref;
}

void GltfScene::progress_callback(const string& message, int current, int total)
{
    CI_LOG_V(message << ": " << current << '/' << total);
}

GltfSceneRef GltfScene::create(const fs::path& path)
{
    auto ref = make_shared<GltfScene>();
    ref->path = path;
    string error;

    if (!load_scene(path.string(), ref->property, error, progress_callback))
    {
        CI_LOG_E(error);
        return {};
    }

    ref->setName(ref->property.asset.name);
    for (auto& shape : ref->property.shapes)
    {
        ref->meshes.emplace_back(ref->createMesh(shape));
    }

    for (auto& texture : ref->property.textures)
    {
        ref->textures.emplace_back(ref->createTexture(texture));
    }

    ref->createMaterials();

    for (auto& instance : ref->property.instances)
    {
        ref->addChild(GltfNode::create(ref.get(), instance));
    }

    return ref;
}

void GltfScene::createMaterials(DebugType debugType)
{
    materials.clear();
    for (auto& material : property.materials)
    {
        materials.emplace_back(GltfMaterial::create(this, material, debugType));
    }
    isMaterialDirty = true;
}

gl::Texture2dRef GltfScene::createTexture(const yocto::scene_texture& texture)
{
    CI_ASSERT(texture.pixelsf.empty());
    return ci::gl::Texture2d::create(texture.pixelsb.data(),
        GL_RGBA, texture.width, texture.height);
}

gl::VboMeshRef GltfScene::createMesh(const yocto::scene_shape& shape)
{
    TriMesh::Format fmt;
    if (!shape.positions.empty()) fmt.positions();
    if (!shape.normals.empty()) fmt.normals();
    if (!shape.tangents.empty()) fmt.tangents();
    if (!shape.texcoords.empty()) fmt.texCoords0();
    if (!shape.colors.empty()) fmt.colors(4);

    TriMesh triMesh(fmt);
    if (!shape.triangles.empty())
        triMesh.appendIndices((uint32_t*)shape.triangles.data(), shape.triangles.size() * 3);
    if (!shape.positions.empty())
        triMesh.appendPositions((glm::vec3*)shape.positions.data(), shape.positions.size());
    if (!shape.tangents.empty())
        triMesh.appendTangents((glm::vec3*)shape.tangents.data(), shape.tangents.size());
    if (!shape.normals.empty())
        triMesh.appendNormals((glm::vec3*)shape.normals.data(), shape.normals.size());
    if (!shape.texcoords.empty())
        triMesh.appendTexCoords0((glm::vec2*)shape.texcoords.data(), shape.texcoords.size());
    if (!shape.colors.empty())
        triMesh.appendColors((ColorA*)shape.colors.data(), shape.colors.size());

    return gl::VboMesh::create(triMesh);
}

