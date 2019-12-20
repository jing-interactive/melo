#include "../include/cigltf.h"
#ifndef CINDER_LESS
#include "AssetManager.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
//#include "cinder/ip/Checkerboard.h"
//#include "../../Remotery/lib/Remotery.h"

using namespace ci;
#else
#include <iostream>
#define CI_ASSERT assert
#define CI_LOG_V(msg) std::cout << msg
#define CI_LOG_F(msg) std::cout << msg
#define CI_LOG_W(msg) std::cout << msg
#define CI_LOG_E(msg) std::cout << msg
#endif
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace melo;

AnimationGLTF::Ref AnimationGLTF::create(ModelGLTFRef modelGLTF,
                                         const tinygltf::Animation& property)
{
    Ref ref = make_shared<AnimationGLTF>();
    ref->property = property;
    return ref;
}

CameraGLTF::Ref CameraGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Camera& property)
{
    Ref ref = make_shared<CameraGLTF>();
    ref->property = property;
    if (property.type == "perspective")
    {
        ref->perspective = property.perspective;
    }
#ifndef CINDER_LESS
    if (property.type == "perspective")
        ref->camera = make_unique<CameraPersp>();
    else
        ref->camera = make_unique<CameraOrtho>();
#endif
    return ref;
}

SamplerGLTF::Ref SamplerGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Sampler& property)
{
    Ref ref = make_shared<SamplerGLTF>();
    ref->property = property;
#ifndef CINDER_LESS
    auto fmt = gl::Sampler::Format()
                   .minFilter((GLenum)property.minFilter)
                   .magFilter((GLenum)property.magFilter)
                   .wrap(property.wrapS, property.wrapT, property.wrapR)
                   .label(property.name);
    ref->ciSampler = gl::Sampler::create(fmt);
    ref->ciSampler->setLabel(property.name);
#endif

    return ref;
}

void PrimitiveGLTF::draw()
{
    if (material)
    {
        material->predraw();
    }
#ifndef CINDER_LESS
    gl::draw(ciVboMesh);
#endif
    if (material)
    {
        material->postdraw();
    }
}

MeshGLTF::Ref MeshGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Mesh& property)
{
    Ref ref = make_shared<MeshGLTF>();
    ref->property = property;
    int primId = 0;
    for (auto& item : property.primitives)
    {
        auto primitive = PrimitiveGLTF::create(modelGLTF, item);
        ref->primitives.emplace_back(primitive);
#ifndef CINDER_LESS
        // Setting labels for vbos and ibo
        char info[256];
        for (auto& kv : primitive->ciVboMesh->getVertexArrayLayoutVbos())
        {
            auto attribInfo = kv.first.getAttribs()[0];
            auto attribName = attribToString(attribInfo.getAttrib());
            snprintf(info, 256, "%s #%d %s", property.name.c_str(), primId, attribName.c_str());
            kv.second->setLabel(info);
        }
        auto ibo = primitive->ciVboMesh->getIndexVbo();
        if (ibo)
        {
            snprintf(info, 256, "%s #%d indices", property.name.c_str(), primId);
            ibo->setLabel(info);
        }
#endif
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

SkinGLTF::Ref SkinGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Skin& property)
{
    Ref ref = make_shared<SkinGLTF>();
    ref->property = property;
    return ref;
}

void NodeGLTF::update(double elapsed)
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

void NodeGLTF::predraw()
{
    //rmt_BeginOpenGLSampleDynamic(getName().c_str());
}

void NodeGLTF::postdraw()
{
    //rmt_EndOpenGLSample();
}


ModelGLTFRef ModelGLTF::create(const fs2::path& meshPath, std::string* loadingError)
{
    if (!fs2::exists(meshPath))
    {
        CI_LOG_F("File doesn't exist: ") << meshPath;
        return {};
    }
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    std::string input_filename(meshPath.string());
    std::string ext = meshPath.extension().string();

    bool ret = false;
    if (ext.compare(".glb") == 0)
    {
        // assume binary glTF.
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
    }
    else
    {
        // assume ascii glTF.
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
    }

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
        CI_LOG_F("Failed to load .glTF ") << meshPath;
        return {};
    }

    ModelGLTFRef ref = make_shared<ModelGLTF>();
    ref->property = model;
    ref->meshPath = meshPath;
    ref->rayCategory = 0xFF;
    ref->setName(meshPath.generic_string());

    {
        tinygltf::Material mtrl;
        mtrl.name = "default";
        mtrl.extensions["KHR_materials_unlit"] = {};
        ref->fallbackMaterial = MaterialGLTF::create(ref, mtrl);
    }

#if 0
    if (model.scenes.size() == 1 && model.scenes[0].name == "OSG_Scene")
    {
        // sanitize gltf from sketchfab
        // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#node
#ifndef CINDER_LESS
        CI_ASSERT_MSG(model.nodes[0].name == "RootNode (gltf orientation matrix)", model.nodes[0].name.c_str());
#endif
        model.nodes[0].rotation = { 0,0,0,1 };

        //CI_ASSERT_MSG(model.nodes[1].name == "RootNode (model correction matrix)", model.nodes[1].name.c_str());
        if (!model.nodes[1].matrix.empty()) model.nodes[1].matrix = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
        if (!model.nodes[2].matrix.empty()) model.nodes[2].matrix = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    }
#endif

    for (auto& item : model.buffers)
        ref->buffers.emplace_back(BufferGLTF::create(ref, item));
    for (auto& item : model.bufferViews)
        ref->bufferViews.emplace_back(BufferViewGLTF::create(ref, item));
    for (auto& item : model.animations)
        ref->animations.emplace_back(AnimationGLTF::create(ref, item));
    for (auto& item : model.accessors)
        ref->accessors.emplace_back(AccessorGLTF::create(ref, item));

    for (auto& item : model.images)
        ref->images.emplace_back(ImageGLTF::create(ref, item));
    for (auto& item : model.samplers)
        ref->samplers.emplace_back(SamplerGLTF::create(ref, item));
    for (auto& item : model.textures)
        ref->textures.emplace_back(TextureGLTF::create(ref, item));
    for (auto& item : model.materials)
        ref->materials.emplace_back(MaterialGLTF::create(ref, item));

    for (auto& item : model.meshes)
        ref->meshes.emplace_back(MeshGLTF::create(ref, item));
    for (auto& item : model.skins)
        ref->skins.emplace_back(SkinGLTF::create(ref, item));
    for (auto& item : model.cameras)
        ref->cameras.emplace_back(CameraGLTF::create(ref, item));

    for (auto& item : model.nodes)
        ref->nodes.emplace_back(NodeGLTF::create(ref, item));
    for (auto& item : model.scenes)
    {
        auto scene = SceneGLTF::create(ref, item);
        ref->scenes.emplace_back(scene);
    }

    if (model.defaultScene == -1)
        model.defaultScene = 0;
    ref->currentScene = ref->scenes[model.defaultScene];

    ref->addChild(ref->currentScene);

    ref->mBoundBoxMin = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
    ref->mBoundBoxMax = {-FLT_MIN, -FLT_MIN, -FLT_MIN};
    for (auto& item : model.accessors)
    {
        if (item.type == TINYGLTF_TYPE_VEC3 && !item.minValues.empty())
        {
            glm::vec3 newMin = { item.minValues[0], item.minValues[1], item.minValues[2] };
            glm::vec3 newMax = { item.maxValues[0], item.maxValues[1], item.maxValues[2] };
            ref->mBoundBoxMax = glm::min(ref->mBoundBoxMax, newMin);
            ref->mBoundBoxMax = glm::max(ref->mBoundBoxMax, newMax);
        }
    }
    ref->treeUpdate();

    return ref;
}

//void ModelGLTF::update(double elapsed)
//{
//    currentScene->treeUpdate(elapsed);
//}

void ModelGLTF::predraw()
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

void ModelGLTF::postdraw()
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

void NodeGLTF::setup()
{
    for (auto& child : property.children)
    {
        addChild(modelGLTF->nodes[child]);
    }
}

NodeGLTF::Ref NodeGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Node& property)
{
    NodeGLTF::Ref ref = make_shared<NodeGLTF>();
    ref->property = property;
    ref->rayCategory = 0xFF;

    if (property.camera != -1)
        ref->camera = modelGLTF->cameras[property.camera];
    if (property.mesh != -1)
    {
        ref->mesh = modelGLTF->meshes[property.mesh];
        ref->setName(ref->mesh->property.name);
    }
    if (property.skin != -1)
        ref->skin = modelGLTF->skins[property.skin];

    if (!property.matrix.empty())
    {
        ref->setConstantTransform(glm::make_mat4x4(property.matrix.data()));
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
        ref->setRotation(glm::make_quat(property.rotation.data()));
    }
    ref->modelGLTF = modelGLTF;

    if (!property.name.empty())
        ref->setName(property.name);

    return ref;
}

SceneGLTF::Ref SceneGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Scene& property)
{
    SceneGLTF::Ref ref = make_shared<SceneGLTF>();
    ref->sceneProperty = property;

    for (auto& item : property.nodes)
    {
        auto child = modelGLTF->nodes[item];
#if 0
        if (property.nodes.size() == 1)
        {
            child->setRotation({});
        }
#endif
        ref->addChild(child);
    }
    if (!property.name.empty())
        ref->setName(property.name);

    return ref;
}

AccessorGLTF::Ref AccessorGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Accessor& property)
{
    // CI_ASSERT_MSG(property.sparse.count == -1, "Unsupported");

    AccessorGLTF::Ref ref = make_shared<AccessorGLTF>();
    auto bufferView = modelGLTF->bufferViews[property.bufferView];
    ref->property = property;
    ref->byteStride = bufferView->property.byteStride;
    ref->cpuBuffer = bufferView->cpuBuffer;
#ifndef CINDER_LESS
    ref->gpuBuffer = bufferView->gpuBuffer;
#endif
    return ref;
}

ImageGLTF::Ref ImageGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Image& property)
{
    ImageGLTF::Ref ref = make_shared<ImageGLTF>();
    ref->property = property;
#ifndef CINDER_LESS
    if (property.image.empty())
        ref->surface = am::surface((modelGLTF->meshPath.parent_path() / property.uri).string());
    else
        ref->surface = Surface::create((uint8_t*)property.image.data(), property.width, property.height,
                                   property.width * property.component,
                                   (property.component == 4) ? SurfaceChannelOrder::RGBA
                                                             : SurfaceChannelOrder::RGB);
#endif
    return ref;
}

BufferGLTF::Ref BufferGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Buffer& property)
{
    BufferGLTF::Ref ref = make_shared<BufferGLTF>();
    ref->property = property;
    ref->cpuBuffer = WeakBuffer::create((void*)ref->property.data.data(), ref->property.data.size());
    return ref;
}

MaterialGLTF::Ref MaterialGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Material& property)
{
    MaterialGLTF::Ref ref = make_shared<MaterialGLTF>();
    ref->property = property;
    ref->modelGLTF = modelGLTF;

    ref->doubleSided = property.doubleSided;
    if (property.alphaMode == "BLEND")
        ref->alphaMode = ALPHA_BLEND;
    else if (property.alphaMode == "MASK")
        ref->alphaMode = ALPHA_MASK;

    for (auto& kv : property.values)
    {
        if (kv.first == "baseColorTexture")
            ref->baseColorTexture = modelGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "baseColorFactor")
        {
            CI_ASSERT(kv.second.number_array.size() == 4);
            ref->baseColorFacor = glm::make_vec4(kv.second.number_array.data());
        }
        else if (kv.first == "metallicRoughnessTexture")
            ref->metallicRoughnessTexture = modelGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "metallicFactor")
            ref->metallicFactor = kv.second.Factor();
        else if (kv.first == "roughnessFactor")
            ref->roughnessFactor = kv.second.Factor();
    }

    for (auto& kv : property.additionalValues)
    {
        if (kv.first == "emissiveTexture")
            ref->emissiveTexture = modelGLTF->textures[kv.second.TextureIndex()];
        else if (kv.first == "emissiveFactor")
        {
            CI_ASSERT(kv.second.number_array.size() == 3);
            ref->emissiveFactor = glm::make_vec3(kv.second.number_array.data());
        }
        else if (kv.first == "normalTexture")
        {
            ref->normalTexture = modelGLTF->textures[kv.second.TextureIndex()];
            auto& jsonValues = kv.second.json_double_value;
            auto it = jsonValues.find("scale");
            if (it != jsonValues.end())
            {
                ref->normalTextureScale = it->second;
            }
        }
        else if (kv.first == "occlusionTexture")
            ref->occlusionTexture = modelGLTF->textures[kv.second.TextureIndex()];
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
                    ref->diffuseTexture = modelGLTF->textures[index];
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
                    ref->metallicRoughnessTexture = modelGLTF->textures[index];
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
            CI_ASSERT(0 && "TODO: support more Material::extensions");
        }
    }
#ifndef CINDER_LESS
    auto& fmt = ref->ciShaderFormat;
    if (ref->baseColorTexture)
        fmt.define("HAS_BASECOLORMAP");
    if (ref->diffuseTexture)
        fmt.define("HAS_DIFFUSEMAP");
    if (ref->metallicRoughnessTexture)
        fmt.define("HAS_METALROUGHNESSMAP");
    if (ref->specularGlossinessTexture)
        fmt.define("HAS_SPECULARGLOSSINESSMAP");
    if (ref->emissiveTexture)
        fmt.define("HAS_EMISSIVEMAP");
    if (ref->normalTexture)
        fmt.define("HAS_NORMALMAP");
    if (ref->occlusionTexture)
        fmt.define("HAS_OCCLUSIONMAP");

    if (Node::radianceTexture && Node::irradianceTexture && Node::brdfLUTTexture)
    {
        fmt.define("HAS_IBL");
        fmt.define("HAS_TEX_LOD");
    }

    if (ref->materialType == MATERIAL_PBR_METAL_ROUGHNESS)
    {
        fmt.vertex(DataSourcePath::create(app::getAssetPath("pbr.vert")));
        fmt.fragment(DataSourcePath::create(app::getAssetPath("pbr.frag")));
        fmt.label("pbr.vert/pbr.frag");
    }
    else if (ref->materialType == MATERIAL_PBR_SPEC_GLOSSINESS)
    {
        fmt.define("PBR_SPECCULAR_GLOSSINESS_WORKFLOW");
        fmt.vertex(DataSourcePath::create(app::getAssetPath("pbr.vert")));
        fmt.fragment(DataSourcePath::create(app::getAssetPath("pbr.frag")));
    }
    else if (ref->materialType == MATERIAL_UNLIT)
    {
        fmt.vertex(DataSourcePath::create(app::getAssetPath("pbr.vert")));
        fmt.fragment(DataSourcePath::create(app::getAssetPath("unlit.frag")));
        fmt.label("pbr.vert/unlit.frag");
    }
#endif
    return ref;
}

#ifdef CINDER_LESS
void MaterialGLTF::predraw() {}
void MaterialGLTF::postdraw() {}

#else
void MaterialGLTF::predraw()
{
    ciShader->uniform("u_flipV", modelGLTF->flipV);
    ciShader->uniform("u_Camera", modelGLTF->cameraPosition);
    ciShader->uniform("u_LightDirection", modelGLTF->lightDirection);
    ciShader->uniform("u_LightColor", modelGLTF->lightColor);

    auto ctx = gl::context();
    if (doubleSided)
    {
        ctx->pushBoolState(GL_CULL_FACE, false);
    }
    if (alphaMode == ALPHA_OPAQUE)
    {
        ctx->pushBoolState(GL_BLEND, false);
    }
    else
    {
        ctx->pushBoolState(GL_BLEND, true);
        ctx->pushBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA,
                                   GL_ONE_MINUS_SRC_ALPHA);
    }

    ciShader->bind();
    if (baseColorTexture)
        baseColorTexture->predraw(0);
    if (diffuseTexture)
        diffuseTexture->predraw(0);
    if (normalTexture)
        normalTexture->predraw(1);
    if (emissiveTexture)
        emissiveTexture->predraw(2);
    if (metallicRoughnessTexture)
        metallicRoughnessTexture->predraw(3);
    if (specularGlossinessTexture)
        specularGlossinessTexture->predraw(3);
    if (occlusionTexture)
        occlusionTexture->predraw(4);
}

void MaterialGLTF::postdraw()
{
    if (doubleSided)
    {
        gl::context()->popBoolState(GL_CULL_FACE);
    }
    gl::context()->popBoolState(GL_BLEND);
    if (alphaMode != ALPHA_OPAQUE)
    {
        gl::context()->popBlendFuncSeparate();
    }
    if (baseColorTexture)
        baseColorTexture->postdraw();
    if (diffuseTexture)
        diffuseTexture->postdraw();
    if (normalTexture)
        normalTexture->postdraw();
    if (emissiveTexture)
        emissiveTexture->postdraw();
    if (metallicRoughnessTexture)
        metallicRoughnessTexture->postdraw();
    if (specularGlossinessTexture)
        specularGlossinessTexture->postdraw();
    if (occlusionTexture)
        occlusionTexture->postdraw();
}
#endif

AttribGLTF getAttribFromString(const string& str)
{
    if (str == "POSITION")
        return POSITION;
    if (str == "COLOR_0")
        return COLOR;
    if (str == "NORMAL")
        return NORMAL;
    if (str == "TANGENT")
        return TANGENT;
    if (str == "BITANGENT")
        return BITANGENT;
    if (str == "JOINTS_0")
        return BONE_INDEX;
    if (str == "WEIGHTS_0")
        return BONE_WEIGHT;

    if (str == "TEXCOORD_0")
        return TEX_COORD_0;
    if (str == "TEXCOORD_1")
        return TEX_COORD_1;
    if (str == "TEXCOORD_2")
        return TEX_COORD_2;
    if (str == "TEXCOORD_3")
        return TEX_COORD_3;

    CI_ASSERT(0 && str.c_str());
    return NUM_ATTRIBS;
}

int32_t getComponentSizeInBytes(GltfComponentType componentType)
{
    if (componentType == COMPONENT_TYPE_BYTE)
        return 1;
    if (componentType == COMPONENT_TYPE_UNSIGNED_BYTE)
        return 1;
    if (componentType == COMPONENT_TYPE_SHORT)
        return 2;
    if (componentType == COMPONENT_TYPE_UNSIGNED_SHORT)
        return 2;
    if (componentType == COMPONENT_TYPE_INT)
        return 4;
    if (componentType == COMPONENT_TYPE_UNSIGNED_INT)
        return 4;
    if (componentType == COMPONENT_TYPE_FLOAT)
        return 4;
    if (componentType == COMPONENT_TYPE_DOUBLE)
        return 8;
    // Unknown componenty type
    return -1;
}
#ifndef CINDER_LESS
geom::DataType getDataType(GltfComponentType componentType)
{
    if (componentType == COMPONENT_TYPE_BYTE)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_UNSIGNED_BYTE)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_SHORT)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_UNSIGNED_SHORT)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_INT)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_UNSIGNED_INT)
        return geom::INTEGER;
    if (componentType == COMPONENT_TYPE_FLOAT)
        return geom::FLOAT;
    if (componentType == COMPONENT_TYPE_DOUBLE)
        return geom::DOUBLE;
    CI_ASSERT(0 && "Unknown componentType");
    return geom::INTEGER;
}
#endif

static inline int32_t getTypeSizeInBytes(GltfType ty)
{
    if (ty == TYPE_SCALAR)
        return 1;
    if (ty == TYPE_VEC2)
        return 2;
    if (ty == TYPE_VEC3)
        return 3;
    if (ty == TYPE_VEC4)
        return 4;
    if (ty == TYPE_MAT2)
        return 4;
    if (ty == TYPE_MAT3)
        return 9;
    if (ty == TYPE_MAT4)
        return 16;
    // Unknown componenty type
    return -1;
}

WeakBufferRef createFromAccessor(AccessorGLTF::Ref acc, GltfType assumedType,
                                 GltfComponentType assumedComponentType)
{
    CI_ASSERT(acc->property.type == (int)assumedType);
    CI_ASSERT(acc->property.componentType == (int)assumedComponentType);

    int typeSize = getTypeSizeInBytes(assumedType);
    int compSize = getComponentSizeInBytes(assumedComponentType);
    auto ref = WeakBuffer::create((uint8_t*)acc->cpuBuffer->getData() + acc->property.byteOffset,
                                  typeSize * compSize * acc->property.count);
    auto ptr = (float*)acc->cpuBuffer->getData();
    ref->type = assumedType;
    ref->componentType = assumedComponentType;

    return ref;
}

PrimitiveGLTF::Ref PrimitiveGLTF::create(ModelGLTFRef modelGLTF,
                                         const tinygltf::Primitive& property)
{
    PrimitiveGLTF::Ref ref = make_shared<PrimitiveGLTF>();
    ref->property = property;
    ref->primitiveMode = (GltfMode)property.mode;

    if (property.material == -1)
    {
        ref->material = modelGLTF->fallbackMaterial;
    }
    else
    {
        ref->material = modelGLTF->materials[property.material];
    }

    const auto& material = ref->material;

    AccessorGLTF::Ref indices;
    if (property.indices >= 0)
    {
        indices = modelGLTF->accessors[property.indices];
    }
#ifdef CINDER_LESS
    if (indices)
    {
        ref->indices = createFromAccessor(indices, TYPE_SCALAR, COMPONENT_TYPE_UNSIGNED_INT);
        ref->indexCount = indices->property.count;
    }

    ref->vertexCount = 0;
    for (auto& kv : property.attributes)
    {
        auto acc = modelGLTF->accessors[kv.second];
        if (kv.first == "POSITION")
            ref->positions = createFromAccessor(acc, TYPE_VEC3, COMPONENT_TYPE_FLOAT);
        if (kv.first == "NORMAL")
            ref->normals = createFromAccessor(acc, TYPE_VEC3, COMPONENT_TYPE_FLOAT);
        if (kv.first == "TEXCOORD_0")
            ref->uvs = createFromAccessor(acc, TYPE_VEC2, COMPONENT_TYPE_FLOAT);
        ref->vertexCount = acc->property.count;
    }
#else

    gl::VboRef oglIndexVbo;
    if (indices)
    {
        if (indices->property.byteOffset == 0)
        {
            oglIndexVbo = indices->gpuBuffer;
            oglIndexVbo->setTarget(GL_ELEMENT_ARRAY_BUFFER);
        }
        else
        {
            int bytesPerUnit = getComponentSizeInBytes((GltfComponentType)indices->property.componentType);
            oglIndexVbo =
                gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, bytesPerUnit * indices->property.count,
                (uint8_t*)indices->cpuBuffer->getData() + indices->property.byteOffset);
        }
    }

    vector<pair<geom::BufferLayout, gl::VboRef>> oglVboLayouts;
    size_t numVertices = 0;
    for (auto& kv : property.attributes)
    {
        AccessorGLTF::Ref acc = modelGLTF->accessors[kv.second];
        geom::BufferLayout layout;
        auto attrib = (geom::Attrib)getAttribFromString(kv.first);
        if (attrib == geom::TEX_COORD_0) material->ciShaderFormat.define("HAS_UV");
        if (attrib == geom::NORMAL) material->ciShaderFormat.define("HAS_NORMALS");
        if (attrib == geom::TANGENT) material->ciShaderFormat.define("HAS_TANGENTS");
        if (attrib == geom::COLOR) material->ciShaderFormat.define("HAS_COLOR");
        layout.append(
            attrib, getDataType((GltfComponentType)acc->property.componentType),
            getTypeSizeInBytes((GltfType)acc->property.type), acc->byteStride, acc->property.byteOffset);
        oglVboLayouts.emplace_back(layout, acc->gpuBuffer);

        numVertices = acc->property.count;
    }

    if (indices)
    {
        ref->ciVboMesh =
            gl::VboMesh::create(numVertices, (GLenum)ref->primitiveMode, oglVboLayouts, indices->property.count,
            (GLenum)indices->property.componentType, oglIndexVbo);
    }
    else
    {

        ref->ciVboMesh =
            gl::VboMesh::create(numVertices, (GLenum)ref->primitiveMode, oglVboLayouts);
    }

    // create shader

    if (!material->ciShader)
    {
#if 1
        material->ciShader = gl::GlslProg::create(material->ciShaderFormat);
#else
        material->ciShader = am::glslProg("lambert texture");
#endif
        auto& ciShader = material->ciShader;
        CI_ASSERT(ciShader && "Shader compile fails");

#ifndef NDEBUG
        auto uniforms = ciShader->getActiveUniforms();
        auto uniformBlocks = ciShader->getActiveUniformBlocks();
        auto attribs = ciShader->getActiveAttributes();
#endif

        if (material->materialType == MATERIAL_PBR_METAL_ROUGHNESS)
        {
            ciShader->uniform("u_BaseColorSampler", 0);
            ciShader->uniform("u_MetallicRoughnessSampler", 3);
        }
        else if (material->materialType == MATERIAL_PBR_SPEC_GLOSSINESS)
        {
            ciShader->uniform("u_DiffuseSampler", 0);
            ciShader->uniform("u_SpecularGlossinessSampler", 3);
        }

        ciShader->uniform("u_LightDirection", vec3(1.0f, 1.0f, 1.0f));
        ciShader->uniform("u_LightColor", vec3(1.0f, 1.0f, 1.0f));

        if (material->normalTexture)
            ciShader->uniform("u_NormalSampler", 1);
        if (material->emissiveTexture)
            ciShader->uniform("u_EmissiveSampler", 2);
        if (material->occlusionTexture)
            ciShader->uniform("u_OcclusionSampler", 4);

        if (modelGLTF->radianceTexture && modelGLTF->irradianceTexture && modelGLTF->brdfLUTTexture)
        {
            ciShader->uniform("u_DiffuseEnvSampler", 5);
            ciShader->uniform("u_SpecularEnvSampler", 6);
            ciShader->uniform("u_brdfLUT", 7);
        }

        ciShader->uniform("u_SpecularGlossinessValues", vec4(material->specularFactor, material->glossinessFactor));
        ciShader->uniform("u_DiffuseFactor", material->diffuseFactor);

        ciShader->uniform("u_MetallicRoughnessValues", vec2(material->metallicFactor, material->roughnessFactor));
        ciShader->uniform("u_BaseColorFactor", material->baseColorFacor);

        ciShader->uniform("u_NormalScale", material->normalTextureScale);
        ciShader->uniform("u_EmissiveFactor", material->emissiveFactor);
        ciShader->uniform("u_OcclusionStrength", material->occlusionStrength);
    }

#endif
    return ref;
}

void PrimitiveGLTF::update() {}

TextureGLTF::Ref TextureGLTF::create(ModelGLTFRef modelGLTF, const tinygltf::Texture& property)
{
    TextureGLTF::Ref ref = make_shared<TextureGLTF>();
    ref->property = property;
    ref->imageSource = modelGLTF->images[property.source];
#ifndef CINDER_LESS
    if (!ref->imageSource->surface) return ref;
    auto texFormat =
        gl::Texture2d::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).wrap(GL_REPEAT);
    ref->ciTexture = gl::Texture2d::create(*ref->imageSource->surface, texFormat);
    if (!ref->ciTexture) return ref;
    ref->ciTexture->setLabel(ref->imageSource->property.uri);

    if (property.sampler != -1)
    {
        auto sampler = modelGLTF->samplers[property.sampler];
        ref->ciSampler = sampler->ciSampler;
    }
    ref->textureUnit = -1;
#endif

    return ref;
}
#ifdef CINDER_LESS
void TextureGLTF::predraw(uint8_t texUnit) {}
void TextureGLTF::postdraw() {}
#else
void TextureGLTF::predraw(uint8_t texUnit)
{
    if (!ciTexture)
        return;

    if (texUnit == -1)
        return;

    textureUnit = texUnit;
    ciTexture->bind(textureUnit);
    if (ciSampler)
        ciSampler->bind(textureUnit);
}

void TextureGLTF::postdraw()
{
    if (!ciTexture)
        return;

    if (textureUnit == -1)
        return;
    ciTexture->unbind(textureUnit);
    if (ciSampler)
        ciSampler->unbind(textureUnit);
    textureUnit = -1;
}
#endif

BufferViewGLTF::Ref BufferViewGLTF::create(ModelGLTFRef modelGLTF,
                                           const tinygltf::BufferView& property)
{
    CI_ASSERT(property.buffer != -1);
    CI_ASSERT(property.byteLength != -1);
    CI_ASSERT(property.byteOffset != -1);
    // CI_ASSERT_MSG(property.byteStride == 0, "TODO: non zero byteStride");

    BufferViewGLTF::Ref ref = make_shared<BufferViewGLTF>();
    ref->property = property;
    ref->target = (GltfTarget)property.target;

    auto buffer = modelGLTF->buffers[property.buffer];
    auto cpuBuffer = buffer->cpuBuffer;
    auto offsetedData = (uint8_t*)cpuBuffer->getData() + property.byteOffset;
    CI_ASSERT(property.byteOffset + property.byteLength <= cpuBuffer->getSize());
    ref->cpuBuffer = WeakBuffer::create(offsetedData, property.byteLength);

#ifndef CINDER_LESS
    GLenum boundTarget = property.target;
    if (boundTarget == 0)
    {
        boundTarget = GL_ARRAY_BUFFER;
    }

    ref->gpuBuffer =
        gl::Vbo::create(boundTarget, ref->cpuBuffer->getSize(), ref->cpuBuffer->getData());
    ref->gpuBuffer->setLabel(property.name);
#endif
    return ref;
}
