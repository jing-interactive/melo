#include "../include/ciobj.h"
#include "AssetManager.h"
#include "MiniConfig.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"

using namespace std;
using namespace melo;

MeshObj::Ref MeshObj::create(ModelObjRef modelObj, const tinyobj::shape_t& property)
{
    auto ref = make_shared<MeshObj>();
    ref->property = property;
    ref->setName(property.name);
    ref->rayCategory = 0xFF;

    const auto& attrib = modelObj->attrib;

    CI_ASSERT_MSG(property.lines.indices.empty(), "TODO: support line");
    CI_ASSERT_MSG(property.points.indices.empty(), "TODO: support points");
    const auto& indices = property.mesh.indices;
    CI_ASSERT_MSG(property.mesh.num_face_vertices.size() == property.mesh.material_ids.size(), "indices.size() is not equal to material_ids.size()");
    CI_ASSERT(!attrib.vertices.empty());

    int i = 0;
    int prevMtrl = -1;
    SubMesh* pSubMesh = nullptr;
    for (const auto& index : indices)
    {
        int mtrl = property.mesh.material_ids[i/3];
        if (mtrl == -1) mtrl = 0;
        if (mtrl != prevMtrl)
        {
            prevMtrl = mtrl;
            if (ref->submeshes.find(mtrl) == ref->submeshes.end())
            {
                ref->submeshes[mtrl] = {};
                ref->submeshes[mtrl].material = modelObj->materials[mtrl];
            }
            pSubMesh = &ref->submeshes[mtrl];
        }

        pSubMesh->indexArray.push_back(pSubMesh->positions.size());

        if (!attrib.vertices.empty())
        {
            pSubMesh->positions.push_back({ attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            });
        }
        if (index.normal_index >= 0 && !attrib.normals.empty())
        {
            pSubMesh->normals.push_back({ attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            });
        }
        if (index.texcoord_index >=0 && !attrib.texcoords.empty())
        {
            pSubMesh->texcoords.push_back({ attrib.texcoords[2 * index.texcoord_index + 0],
                attrib.texcoords[2 * index.texcoord_index + 1]
            });
        }
        if (!attrib.colors.empty())
        {
            pSubMesh->colors.push_back({ attrib.colors[3 * index.vertex_index + 0],
                attrib.colors[3 * index.vertex_index + 1],
                attrib.colors[3 * index.vertex_index + 2]
            });
        }
        i++;
    }

    ref->mBoundBoxMin = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
    ref->mBoundBoxMax = { -FLT_MIN, -FLT_MIN, -FLT_MIN };
    for (auto& kv : ref->submeshes)
    {
        auto& submesh = kv.second;
        submesh.setup();
        ref->mBoundBoxMin = glm::min(submesh.boundBoxMin, ref->mBoundBoxMin);
        ref->mBoundBoxMax = glm::max(submesh.boundBoxMax, ref->mBoundBoxMax);
    }

    return ref;
}

void MeshObj::SubMesh::setup()
{
    TriMesh::Format fmt;
    fmt.positions();
    fmt.normals();
    if (!texcoords.empty()) fmt.texCoords();
    if (!colors.empty()) fmt.colors();
    TriMesh triMesh(fmt);

    triMesh.appendPositions(positions.data(), positions.size());
    if (!normals.empty())
        triMesh.appendNormals(normals.data(), normals.size());
    if (!texcoords.empty())
        triMesh.appendTexCoords0(texcoords.data(), texcoords.size());
    if (!colors.empty())
        triMesh.appendColors(colors.data(), colors.size());
    triMesh.appendIndices(indexArray.data(), indexArray.size());
    if (normals.empty())
    {
        triMesh.recalculateNormals();
    }
    triMesh.recalculateTangents();
    boundBoxMin = triMesh.calcBoundingBox().getMin();
    boundBoxMax = triMesh.calcBoundingBox().getMax();

    positions.clear();
    texcoords.clear();
    colors.clear();
    normals.clear();
    indexArray.clear();

    vboMesh = gl::VboMesh::create(triMesh);
}

void MeshObj::SubMesh::draw()
{
    material->predraw();
    gl::draw(vboMesh);
    material->postdraw();
}

void MeshObj::predraw()
{
#ifndef CINDER_LESS
    if (Node::irradianceTexture && Node::radianceTexture && Node::brdfLUTTexture)
    {
        Node::irradianceTexture->bind(5);
        Node::radianceTexture->bind(6);
        Node::brdfLUTTexture->bind(7);
    }
#endif
}

void MeshObj::postdraw()
{
#ifndef CINDER_LESS
    if (Node::irradianceTexture && Node::radianceTexture && Node::brdfLUTTexture)
    {
        Node::irradianceTexture->unbind(5);
        Node::radianceTexture->unbind(6);
        Node::brdfLUTTexture->unbind(7);
    }
#endif
}

void MeshObj::draw()
{
    for (auto& kv : submeshes)
    {
        kv.second.draw();
    }
}

void MaterialObj::predraw()
{
    ciShader->bind();
    if (diffuseTexture)
        diffuseTexture->bind(0);

    ciShader->uniform("u_flipV", modelObj->flipV);
    ciShader->uniform("u_Camera", modelObj->cameraPosition);
    ciShader->uniform("u_LightDirection", modelObj->lightDirection);
    ciShader->uniform("u_LightColor", modelObj->lightColor);
}

void MaterialObj::postdraw()
{
    if (diffuseTexture)
        diffuseTexture->unbind(0);
}

MaterialObj::Ref MaterialObj::create(ModelObjRef modelObj, const tinyobj::material_t& property)
{
    auto ref = make_shared<MaterialObj>();
    ref->property = property;
    ref->modelObj = modelObj;
    ref->name = property.name;
    ref->materialType = MATERIAL_PBR_SPEC_GLOSSINESS;

    auto fmt = gl::GlslProg::Format();

    if (ref->materialType == MATERIAL_PBR_SPEC_GLOSSINESS)
        fmt.define("PBR_SPECCULAR_GLOSSINESS_WORKFLOW");

    fmt.define("HAS_TANGENTS");
    fmt.define("HAS_NORMALS");

    //if (ref->baseColorTexture)
    //    fmt.define("HAS_BASECOLORMAP");
    //if (ref->metallicRoughnessTexture)
    //    fmt.define("HAS_METALROUGHNESSMAP");
    //if (ref->emissiveTexture)
    //    fmt.define("HAS_EMISSIVEMAP");
    //if (ref->occlusionTexture)
    //    fmt.define("HAS_OCCLUSIONMAP");

    if (!property.diffuse_texname.empty())
    {
        auto path = modelObj->baseDir / property.diffuse_texname;
        ref->diffuseTexture = am::texture2d(path.string());
    }

    if (!property.bump_texname.empty())
    {
        auto path = modelObj->baseDir / property.bump_texname;
        ref->normalTexture = am::texture2d(path.string());
    }

    if (ref->diffuseTexture)
    {
        fmt.define("HAS_UV");
        fmt.define("HAS_DIFFUSEMAP");
    }
    if (ref->normalTexture)
        fmt.define("HAS_NORMALMAP");

#if 0
    ref->diffuseFactor = glm::vec4(0.5, 0.5, 0.5, 1);
#else
    ref->diffuseFactor = { property.diffuse[0], property.diffuse[1], property.diffuse[2], 1 };
#endif
    //ref->ambientFactor = { property.ambient[0], property.ambient[1], property.ambient[2] };
    ref->specularFactor = { property.specular[0], property.specular[1], property.diffuse[2] };
    ref->emissiveFactor = { property.emission[0], property.emission[1], property.emission[2] };
    ref->glossinessFactor = property.shininess;

    if (Node::radianceTexture && Node::irradianceTexture && Node::brdfLUTTexture)
    {
        fmt.define("HAS_IBL");
        fmt.define("HAS_TEX_LOD");
    }

    fmt.vertex(DataSourcePath::create(app::getAssetPath("pbr.vert")));
    fmt.fragment(DataSourcePath::create(app::getAssetPath("pbr.frag")));
    fmt.label("pbr.vert/pbr.frag");

    // use stock shader for the moment
#if 1
    auto ciShader = gl::GlslProg::create(fmt);
#else
    auto ciShader = am::glslProg("lambert texture");
#endif
    CI_ASSERT_MSG(ciShader, "Shader compile fails");

#ifndef NDEBUG
    auto uniforms = ciShader->getActiveUniforms();
    auto uniformBlocks = ciShader->getActiveUniformBlocks();
    auto attribs = ciShader->getActiveAttributes();
#endif

    if (ref->materialType == MATERIAL_PBR_METAL_ROUGHNESS)
    {
        ciShader->uniform("u_BaseColorSampler", 0);
        ciShader->uniform("u_MetallicRoughnessSampler", 3);
    }
    else if (ref->materialType == MATERIAL_PBR_SPEC_GLOSSINESS)
    {
        ciShader->uniform("u_DiffuseSampler", 0);
        ciShader->uniform("u_SpecularGlossinessSampler", 3);
    }

    if (ref->normalTexture)
    {
        ciShader->uniform("u_NormalSampler", 1);
        ciShader->uniform("u_NormalScale", 1.0f);
    }

    if (Node::radianceTexture && Node::irradianceTexture && Node::brdfLUTTexture)
    {
        ciShader->uniform("u_DiffuseEnvSampler", 5);
        ciShader->uniform("u_SpecularEnvSampler", 6);
        ciShader->uniform("u_brdfLUT", 7);
    }

    ciShader->uniform("u_DiffuseFactor", ref->diffuseFactor);
    ciShader->uniform("u_SpecularGlossinessValues", vec4(ref->specularFactor, ref->glossinessFactor));
    ciShader->uniform("u_EmissiveFactor", ref->emissiveFactor);

    ciShader->uniform("u_LightDirection", vec3(1.0f, 1.0f, 1.0f));
    ciShader->uniform("u_LightColor", vec3(1.0f, 1.0f, 1.0f));
    ciShader->uniform("u_Camera", vec3(1.0f, 1.0f, 1.0f));

    if (ref->normalTexture)
    ciShader->uniform("u_OcclusionStrength", 1.0f);

    //if (ref->emissiveTexture)
    //    ciShader->uniform("u_EmissiveSampler", 2);
    //if (ref->occlusionTexture)
    //    ciShader->uniform("u_OcclusionSampler", 4);

    ref->ciShader = ciShader;

    return ref;
}

ModelObjRef ModelObj::create(const fs::path& meshPath, std::string* loadingError)
{
    if (!fs::exists(meshPath))
    {
        CI_LOG_F("File doesn't exist: ") << meshPath;
        return{};
    }

    auto ref = make_shared<ModelObj>();
    ref->meshPath = meshPath;
    ref->setName(meshPath.string());
    ref->baseDir = meshPath.parent_path().string();

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&ref->attrib, &shapes, &materials, &warn, &err,
        meshPath.string().c_str(), ref->baseDir.string().c_str());
    if (!warn.empty())
    {
        CI_LOG_W(warn);
    }
    if (!err.empty())
    {
        CI_LOG_E(err);
    }
    if (loadingError) *loadingError = err;

    if (!ret)
    {
        CI_LOG_E("Failed to load ") << meshPath;
        return{};
    }

    CI_LOG_I("# of vertices  ") << (ref->attrib.vertices.size() / 3);
    CI_LOG_I("# of normals   ") << (ref->attrib.normals.size() / 3);
    CI_LOG_I("# of texcoords ") << (ref->attrib.texcoords.size() / 2);
    CI_LOG_I("# of materials ") << materials.size();
    CI_LOG_I("# of shapes    ") << shapes.size();

    // Append `default` material
    materials.push_back(tinyobj::material_t());
    for (auto& item : materials)
        ref->materials.emplace_back(MaterialObj::create(ref, item));

    for (auto& item : shapes)
    {
        auto mesh = MeshObj::create(ref, item);

        ref->mBoundBoxMin = glm::min(mesh->mBoundBoxMin, ref->mBoundBoxMin);
        ref->mBoundBoxMax = glm::max(mesh->mBoundBoxMax, ref->mBoundBoxMax);
        ref->addChild(mesh);
    }

    ref->rayCategory = 0xFF;

    return ref;
}

