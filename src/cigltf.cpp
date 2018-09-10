#include "cigltf.h"
#include "AssetManager.h"
#include "MiniConfig.h"
#include "cinder/Log.h"
#include <xutility>

using namespace std;

AnimationGLTF::Ref AnimationGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Animation& property)
{
    Ref ref = make_shared<AnimationGLTF>();
    ref->property = property;
    return ref;
}

CameraGLTF::Ref CameraGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Camera& property)
{
    Ref ref = make_shared<CameraGLTF>();
    ref->property = property;
    if (property.type == "perspective")
        ref->camera = make_unique<CameraPersp>();
    else
        ref->camera = make_unique<CameraOrtho>();
    return ref;
}

SamplerGLTF::Ref SamplerGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Sampler& property)
{
    Ref ref = make_shared<SamplerGLTF>();
    ref->property = property;

    ref->ciFormat.minFilter((GLenum)property.minFilter)
        .magFilter((GLenum)property.magFilter)
        .wrap(property.wrapS, property.wrapT, property.wrapR);

    return ref;
}

void PrimitiveGLTF::draw()
{
    if (material)
    {
        material->preDraw();
    }

    gl::draw(ciVboMesh);

    if (material)
    {
        material->postDraw();
    }
}

MeshGLTF::Ref MeshGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Mesh& property)
{
    Ref ref = make_shared<MeshGLTF>();
    ref->property = property;
    int primId = 0;
    for (auto& item : property.primitives)
    {
        auto primitive = PrimitiveGLTF::create(rootGLTF, item);
        ref->primitives.emplace_back(primitive);

        // Setting labels for vbos and ibo
        int vboId = 0;
        char info[100];
        for (auto& kv : primitive->ciVboMesh->getVertexArrayLayoutVbos())
        {
            auto attribInfo = kv.first.getAttribs()[0];
            auto attribName = attribToString(attribInfo.getAttrib());
            sprintf(info, "%s #%d %s", property.name.c_str(), primId, attribName.c_str());
            kv.second->setLabel(info);
            vboId++;
        }
        sprintf(info, "%s #%d indices", property.name.c_str(), primId);
        primitive->ciVboMesh->getIndexVbo()->setLabel(info);
    }

    return ref;
}

void MeshGLTF::update()
{
    for (auto& item : primitives)
        item->update();
}

void MeshGLTF::draw()
{
    for (auto& item : primitives)
        item->draw();
}

SkinGLTF::Ref SkinGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Skin& property)
{
    Ref ref = make_shared<SkinGLTF>();
    ref->property = property;
    return ref;
}

void NodeGLTF::update()
{
    if (mesh)
    {
        mesh->update();
    }
}

void NodeGLTF::draw()
{
    if (mesh)
    {
        mesh->draw();
    }
}

RootGLTFRef RootGLTF::create(const fs::path& gltfPath)
{
    if (!fs::exists(gltfPath))
    {
        CI_LOG_F("File doesn't exist: ") << gltfPath;
        return {};
    }
    tinygltf::TinyGLTF loader;
    tinygltf::Model root;
    std::string err;
    std::string warn;
    std::string input_filename(gltfPath.string());
    std::string ext = gltfPath.extension().string();

    bool ret = false;
    if (ext.compare(".glb") == 0)
    {
        // assume binary glTF.
        ret = loader.LoadBinaryFromFile(&root, &err, &warn, input_filename.c_str());
    }
    else
    {
        // assume ascii glTF.
        ret = loader.LoadASCIIFromFile(&root, &err, &warn, input_filename.c_str());
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
        CI_LOG_F("Failed to load .glTF") << gltfPath;
        return {};
    }

    RootGLTFRef ref = make_shared<RootGLTF>();
    ref->property = root;
    ref->gltfPath = gltfPath;

    ref->radianceTexture = am::textureCubeMap(RADIANCE_TEX);
    ref->irradianceTexture = am::textureCubeMap(IRRADIANCE_TEX);
    ref->brdfLUTTexture = am::texture2d(BRDF_LUT_TEX);

    for (auto& item : root.buffers)
        ref->buffers.emplace_back(BufferGLTF::create(ref, item));
    for (auto& item : root.bufferViews)
        ref->bufferViews.emplace_back(BufferViewGLTF::create(ref, item));
    for (auto& item : root.animations)
        ref->animations.emplace_back(AnimationGLTF::create(ref, item));
    for (auto& item : root.accessors)
        ref->accessors.emplace_back(AccessorGLTF::create(ref, item));

    for (auto& item : root.images)
        ref->images.emplace_back(ImageGLTF::create(ref, item));
    for (auto& item : root.samplers)
        ref->samplers.emplace_back(SamplerGLTF::create(ref, item));
    for (auto& item : root.textures)
        ref->textures.emplace_back(TextureGLTF::create(ref, item));
    for (auto& item : root.materials)
        ref->materials.emplace_back(MaterialGLTF::create(ref, item));

    for (auto& item : root.meshes)
        ref->meshes.emplace_back(MeshGLTF::create(ref, item));
    for (auto& item : root.skins)
        ref->skins.emplace_back(SkinGLTF::create(ref, item));
    for (auto& item : root.cameras)
        ref->cameras.emplace_back(CameraGLTF::create(ref, item));

    for (auto& item : root.nodes)
        ref->nodes.emplace_back(NodeGLTF::create(ref, item));
    for (auto& item : root.scenes)
        ref->scenes.emplace_back(SceneGLTF::create(ref, item));

    if (root.defaultScene == -1)
        root.defaultScene = 0;
    ref->currentScene = ref->scenes[root.defaultScene];

    return ref;
}

void RootGLTF::update() { currentScene->treeUpdate(); }

void RootGLTF::draw()
{
    gl::ScopedTextureBind scpIrr(irradianceTexture, 5);
    gl::ScopedTextureBind scpRad(radianceTexture, 6);
    gl::ScopedTextureBind scpBrdf(brdfLUTTexture, 7);

    currentScene->treeDraw();
}

void NodeGLTF::setup()
{
    for (auto& child : property.children)
    {
        addChild(rootGLTF->nodes[child]);
    }
}

NodeGLTF::Ref NodeGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Node& property)
{
    NodeGLTF::Ref ref = make_shared<NodeGLTF>();
    ref->property = property;
    if (property.camera != -1)
        ref->camera = rootGLTF->cameras[property.camera];
    if (property.mesh != -1)
    {
        ref->mesh = rootGLTF->meshes[property.mesh];
        ref->setName(ref->mesh->property.name);
    }
    if (property.skin != -1)
        ref->skin = rootGLTF->skins[property.skin];

    if (!property.matrix.empty())
    {
        ref->setTransform(glm::make_mat4x4(property.matrix.data()));
    }
    if (!property.translation.empty())
    {
        ref->setPosition(
            {property.translation[0], property.translation[1], property.translation[2]});
    }
    if (!property.scale.empty())
    {
        ref->setScale({property.scale[0], property.scale[1], property.scale[2]});
    }
    if (!property.rotation.empty())
    {
        ref->setRotation({(float)property.rotation[3], (float)property.rotation[0],
                          (float)property.rotation[1],
                          (float)property.rotation[2]}); // (w, x, y, z)
    }
    ref->rootGLTF = rootGLTF;

    if (!property.name.empty())
        ref->setName(property.name);

    return ref;
}

SceneGLTF::Ref SceneGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Scene& property)
{
    SceneGLTF::Ref ref = make_shared<SceneGLTF>();
    ref->sceneProperty = property;

    for (auto& item : property.nodes)
    {
        ref->addChild(rootGLTF->nodes[item]);
    }

    return ref;
}

AccessorGLTF::Ref AccessorGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Accessor& property)
{
    // CI_ASSERT_MSG(property.sparse.count == -1, "Unsupported");

    AccessorGLTF::Ref ref = make_shared<AccessorGLTF>();
    auto bufferView = rootGLTF->bufferViews[property.bufferView];
    ref->property = property;
    ref->byteStride = bufferView->property.byteStride;
    ref->gpuBuffer = bufferView->gpuBuffer;

    return ref;
}

ImageGLTF::Ref ImageGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Image& property)
{
    ImageGLTF::Ref ref = make_shared<ImageGLTF>();
    ref->property = property;

#if 0
    ref->surface = am::surface((rootGLTF->gltfPath.parent_path() / property.uri).string());
#else
    ref->surface = Surface::create((uint8_t*)property.image.data(), property.width, property.height,
                                   property.width * property.component,
                                   (property.component == 4) ? SurfaceChannelOrder::RGBA
                                                             : SurfaceChannelOrder::RGB);
#endif

    return ref;
}

BufferGLTF::Ref BufferGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Buffer& property)
{
    BufferGLTF::Ref ref = make_shared<BufferGLTF>();
    ref->property = property;

#if 0
    ref->cpuBuffer = am::buffer((rootGLTF->gltfPath.parent_path() / property.uri).string());
#else
    ref->cpuBuffer = Buffer::create((void*)property.data.data(), property.data.size());
#endif

    return ref;
}

MaterialGLTF::Ref MaterialGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Material& property)
{
    MaterialGLTF::Ref ref = make_shared<MaterialGLTF>();
    ref->property = property;
    ref->rootGLTF = rootGLTF;

    ref->doubleSided = false;

    for (auto& kv : property.values)
    {
        if (kv.first == "baseColorTexture")
            ref->baseColorTexture = rootGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "baseColorFactor")
        {
            CI_ASSERT(kv.second.number_array.size() == 4);
            ref->baseColorFacor = glm::make_vec4(kv.second.number_array.data());
        }
        else if (kv.first == "metallicRoughnessTexture")
            ref->metallicRoughnessTexture = rootGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "metallicFactor")
            ref->metallicFactor = kv.second.Factor();
        else if (kv.first == "roughnessFactor")
            ref->roughnessFactor = kv.second.Factor();
    }

    for (auto& kv : property.additionalValues)
    {
        if (kv.first == "emissiveTexture")
            ref->emissiveTexture = rootGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "emissiveFactor")
        {
            CI_ASSERT(kv.second.number_array.size() == 3);
            ref->emissiveFactor = glm::make_vec3(kv.second.number_array.data());
        }
        else if (kv.first == "normalTexture")
        {
            ref->normalTexture = rootGLTF->textures[kv.second.TextureIndex()];
            auto& jsonValues = kv.second.json_double_value;
            auto it = jsonValues.find("scale");
            if (it != std::end(jsonValues))
            {
                ref->normalTextureScale = it->second;
            }
        }
        else if (kv.first == "occlusionTexture")
            ref->occlusionTexture = rootGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "doubleSided")
            ref->doubleSided = kv.second.bool_value;
    }

    ref->materialType = MATERIAL_PBR_METAL_ROUGHNESS;
    for (auto& ext : property.extensions)
    {
        if (ext.first == "KHR_materials_unlit")
        {
            ref->materialType = MATERIAL_UNLIT;
        }
        else if (ext.first == "KHR_materials_pbrSpecularGlossiness")
        {
            ref->materialType = MATERIAL_PBR_SPEC_GLOSSINESS;
            CI_ASSERT(ext.second.IsObject());
            const auto& fields = ext.second.Get<tinygltf::Value::Object>();
            for (auto& kv : fields)
            {
                if (kv.first == "diffuseTexture")
                {
                    CI_ASSERT(kv.second.IsObject());
                    auto obj = kv.second.Get<tinygltf::Value::Object>();
                    int index = obj["index"].Get<int>();
                    ref->diffuseTexture = rootGLTF->textures[index];
                    if (obj.find("texCoord") != obj.end())
                    {
                        ref->diffuseTextureCoord = obj["texCoord"].Get<int>();
                    }
                }
                else if (kv.first == "specularGlossinessTexture")
                {
                    CI_ASSERT(kv.second.IsObject());
                    auto obj = kv.second.Get<tinygltf::Value::Object>();
                    int index = obj["index"].Get<int>();
                    ref->specularGlossinessTexture = rootGLTF->textures[index];
                    // if (obj.find("texCoord") != obj.end())
                    //{
                    //    ref->specularGlossinessTexture = obj["texCoord"].Get<int>();
                    //}
                }
                else if (kv.first == "diffuseFactor")
                {
                    CI_ASSERT(kv.second.ArrayLen() == 4);
                    auto& arr = kv.second.Get<tinygltf::Value::Array>();
                    if (arr[0].IsInt())
                    {
                        ref->diffuseFactor.r = arr[0].Get<int>();
                        ref->diffuseFactor.g = arr[1].Get<int>();
                        ref->diffuseFactor.b = arr[2].Get<int>();
                        ref->diffuseFactor.a = arr[3].Get<int>();
                    }
                    else if (arr[0].IsNumber())
                    {
                        ref->diffuseFactor.r = arr[0].Get<double>();
                        ref->diffuseFactor.g = arr[1].Get<double>();
                        ref->diffuseFactor.b = arr[2].Get<double>();
                        ref->diffuseFactor.a = arr[3].Get<double>();
                    }
                }
                else if (kv.first == "specularFactor")
                {
                    CI_ASSERT(kv.second.ArrayLen() >= 3);
                    auto& arr = kv.second.Get<tinygltf::Value::Array>();
                    if (arr[0].IsInt())
                    {
                        ref->specularFactor.r = arr[0].Get<int>();
                        ref->specularFactor.g = arr[1].Get<int>();
                        ref->specularFactor.b = arr[2].Get<int>();
                    }
                    else if (arr[0].IsNumber())
                    {
                        ref->specularFactor.r = arr[0].Get<double>();
                        ref->specularFactor.g = arr[1].Get<double>();
                        ref->specularFactor.b = arr[2].Get<double>();
                    }
                }
                else if (kv.first == "glossinessFactor")
                {
                    if (kv.second.IsInt())
                    {
                        ref->glossinessFactor = kv.second.Get<int>();
                    }
                    else if (kv.second.IsNumber())
                    {
                        ref->glossinessFactor = kv.second.Get<double>();
                    }
                }
            }
        }
        else
        {
            CI_ASSERT_MSG(0, "TODO: support more Material::extensions");
        }
    }

    auto fmt = gl::GlslProg::Format();
    fmt.define("HAS_UV");
    fmt.define("HAS_TANGENTS");
    fmt.define("HAS_NORMALS");
    if (ref->baseColorTexture)
        fmt.define("HAS_BASECOLORMAP");
    if (ref->metallicRoughnessTexture)
        fmt.define("HAS_METALROUGHNESSMAP");
    if (ref->diffuseTexture)
        fmt.define("HAS_DIFFUSEMAP");
    if (ref->specularGlossinessTexture)
        fmt.define("HAS_SPECULARGLOSSINESSMAP");
    if (ref->emissiveTexture)
        fmt.define("HAS_EMISSIVEMAP");
    if (ref->normalTexture)
        fmt.define("HAS_NORMALMAP");
    if (ref->occlusionTexture)
        fmt.define("HAS_OCCLUSIONMAP");

    if (rootGLTF->radianceTexture && rootGLTF->irradianceTexture && rootGLTF->brdfLUTTexture)
    {
        fmt.define("HAS_IBL");
        fmt.define("HAS_TEX_LOD");
    }

    gl::GlslProgRef ciShader;

    if (ref->materialType == MATERIAL_PBR_METAL_ROUGHNESS)
    {
        ciShader = am::glslProg("pbr.vert", "pbr.frag", fmt);
    }
    else if (ref->materialType == MATERIAL_PBR_SPEC_GLOSSINESS)
    {
        fmt.define("PBR_SPECCULAR_GLOSSINESS_WORKFLOW");
        ciShader = am::glslProg("pbr.vert", "pbr.frag", fmt);
    }
    else if (ref->materialType == MATERIAL_UNLIT)
    {
        ciShader = am::glslProg("texture");
    }
    CI_ASSERT_MSG(ciShader, "Shader compile fails");
    ref->ciShader = ciShader;

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
    ciShader->uniform("u_NormalSampler", 1);
    ciShader->uniform("u_EmissiveSampler", 2);
    ciShader->uniform("u_OcclusionSampler", 4);

    ciShader->uniform("u_DiffuseEnvSampler", 5);
    ciShader->uniform("u_SpecularEnvSampler", 6);
    ciShader->uniform("u_brdfLUT", 7);

    return ref;
}

void MaterialGLTF::preDraw()
{
    ciShader->uniform("u_flipV", rootGLTF->flipV);
    ciShader->uniform("u_Camera", rootGLTF->cameraPosition);

    ciShader->uniform("u_SpecularGlossinessValues", vec4(specularFactor, glossinessFactor));
    ciShader->uniform("u_DiffuseFactor", diffuseFactor);

    ciShader->uniform("u_MetallicRoughnessValues", vec2(metallicFactor, roughnessFactor));
    ciShader->uniform("u_BaseColorFactor", baseColorFacor);

    ciShader->uniform("u_NormalScale", normalTextureScale);
    ciShader->uniform("u_EmissiveFactor", emissiveFactor);
    ciShader->uniform("u_OcclusionStrength", occlusionStrength);

    ciShader->bind();
    if (baseColorTexture)
        baseColorTexture->preDraw(0);
    if (normalTexture)
        normalTexture->preDraw(1);
    if (emissiveTexture)
        emissiveTexture->preDraw(2);
    if (metallicRoughnessTexture)
        metallicRoughnessTexture->preDraw(3);
    if (occlusionTexture)
        occlusionTexture->preDraw(4);
}

void MaterialGLTF::postDraw()
{
    if (baseColorTexture)
        baseColorTexture->postDraw();
    if (normalTexture)
        normalTexture->postDraw();
    if (emissiveTexture)
        emissiveTexture->postDraw();
    if (metallicRoughnessTexture)
        metallicRoughnessTexture->postDraw();
    if (occlusionTexture)
        occlusionTexture->postDraw();
}

geom::Attrib getAttribFromString(const string& str)
{
    if (str == "POSITION")
        return geom::POSITION;
    if (str == "NORMAL")
        return geom::NORMAL;
    if (str == "TANGENT")
        return geom::TANGENT;
    if (str == "BITANGENT")
        return geom::BITANGENT;
    if (str == "JOINT")
        return geom::BONE_INDEX;
    if (str == "WEIGHT")
        return geom::BONE_WEIGHT;

    if (str == "TEXCOORD_0")
        return geom::TEX_COORD_0;
    if (str == "TEXCOORD_1")
        return geom::TEX_COORD_1;
    if (str == "TEXCOORD_2")
        return geom::TEX_COORD_2;
    if (str == "TEXCOORD_3")
        return geom::TEX_COORD_3;

    if (str == "COLOR_0")
        return geom::COLOR;

    CI_ASSERT_MSG(0, str.c_str());
    return geom::USER_DEFINED;
}

geom::DataType getDataType(uint32_t componentType)
{
    if (componentType == TINYGLTF_COMPONENT_TYPE_BYTE)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_INT)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        return geom::INTEGER;
    if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        return geom::FLOAT;
    if (componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE)
        return geom::DOUBLE;
    CI_ASSERT_MSG(0, "Unknown componentType");
    return geom::INTEGER;
}

static inline int32_t GetTypeSizeInBytes(uint32_t ty)
{
    if (ty == TINYGLTF_TYPE_SCALAR)
    {
        return 1;
    }
    else if (ty == TINYGLTF_TYPE_VEC2)
    {
        return 2;
    }
    else if (ty == TINYGLTF_TYPE_VEC3)
    {
        return 3;
    }
    else if (ty == TINYGLTF_TYPE_VEC4)
    {
        return 4;
    }
    else if (ty == TINYGLTF_TYPE_MAT2)
    {
        return 4;
    }
    else if (ty == TINYGLTF_TYPE_MAT3)
    {
        return 9;
    }
    else if (ty == TINYGLTF_TYPE_MAT4)
    {
        return 16;
    }
    else
    {
        // Unknown componenty type
        return -1;
    }
}

PrimitiveGLTF::Ref PrimitiveGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Primitive& property)
{
    PrimitiveGLTF::Ref ref = make_shared<PrimitiveGLTF>();
    ref->property = property;

    AccessorGLTF::Ref indices = rootGLTF->accessors[property.indices];
    ref->material = rootGLTF->materials[property.material];

    GLenum oglPrimitiveMode = (GLenum)property.mode;
    auto oglIndexVbo = indices->gpuBuffer;
    oglIndexVbo->setTarget(GL_ELEMENT_ARRAY_BUFFER);

    vector<pair<geom::BufferLayout, gl::VboRef>> oglVboLayouts;
    size_t numVertices = 0;
    for (auto& kv : property.attributes)
    {
        AccessorGLTF::Ref acc = rootGLTF->accessors[kv.second];
        geom::BufferLayout layout;
        layout.append(getAttribFromString(kv.first), getDataType(acc->property.componentType),
                      GetTypeSizeInBytes(acc->property.type), acc->byteStride,
                      acc->property.byteOffset);
        oglVboLayouts.emplace_back(layout, acc->gpuBuffer);

        numVertices = acc->property.count;
    }

    ref->ciVboMesh =
        gl::VboMesh::create(numVertices, oglPrimitiveMode, oglVboLayouts, indices->property.count,
                            (GLenum)indices->property.componentType, oglIndexVbo);

    return ref;
}

void PrimitiveGLTF::update() {}

TextureGLTF::Ref TextureGLTF::create(RootGLTFRef rootGLTF, const tinygltf::Texture& property)
{
    TextureGLTF::Ref ref = make_shared<TextureGLTF>();
    ref->property = property;

    ImageGLTF::Ref source = rootGLTF->images[property.source];
    auto texFormat =
        gl::Texture2d::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).wrap(GL_REPEAT);
    ref->ciTexture = gl::Texture2d::create(*source->surface, texFormat);
    ref->ciTexture->setLabel(source->property.uri);

    if (property.sampler != -1)
    {
        auto sampler = rootGLTF->samplers[property.sampler];
        ref->ciSampler = gl::Sampler::create(sampler->ciFormat);
        ref->ciSampler->setLabel(sampler->property.name);
    }

    ref->textureUnit = -1;

    return ref;
}

void TextureGLTF::preDraw(uint8_t texUnit)
{
    if (texUnit == -1)
        return;

    textureUnit = texUnit;
    ciTexture->bind(textureUnit);
    if (ciSampler)
        ciSampler->bind(textureUnit);
}

void TextureGLTF::postDraw()
{
    if (textureUnit == -1)
        return;

    ciTexture->unbind(textureUnit);
    if (ciSampler)
        ciSampler->unbind(textureUnit);
    textureUnit = -1;
}

BufferViewGLTF::Ref BufferViewGLTF::create(RootGLTFRef rootGLTF,
                                           const tinygltf::BufferView& property)
{
    CI_ASSERT(property.buffer != -1);
    CI_ASSERT(property.byteLength != -1);
    CI_ASSERT(property.byteOffset != -1);
    // CI_ASSERT_MSG(property.byteStride == 0, "TODO: non zero byteStride");

    BufferViewGLTF::Ref ref = make_shared<BufferViewGLTF>();
    ref->property = property;

    auto buffer = rootGLTF->buffers[property.buffer];
    auto cpuBuffer = buffer->cpuBuffer;
    auto offsetedData = (uint8_t*)cpuBuffer->getData() + property.byteOffset;
    CI_ASSERT(property.byteOffset + property.byteLength <= cpuBuffer->getSize());

    GLenum boundTarget = property.target;
    if (boundTarget == 0)
    {
        boundTarget = GL_ARRAY_BUFFER;
    }

    ref->cpuBuffer = Buffer::create(offsetedData, property.byteLength);
    ref->gpuBuffer =
        gl::Vbo::create(boundTarget, ref->cpuBuffer->getSize(), ref->cpuBuffer->getData());
    ref->gpuBuffer->setLabel(property.name);

    return ref;
}
