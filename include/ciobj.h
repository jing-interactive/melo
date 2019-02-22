#pragma once

#include "../3rdparty/tinyobjloader/tiny_obj_loader.h"

#include "Node3D.h"

#include <cinder/gl/gl.h>
#include <memory>
#include <vector>

typedef std::shared_ptr<struct ModelObj> ModelObjRef;

using namespace ci;

struct MaterialObj
{
    typedef std::shared_ptr<MaterialObj> Ref;
    std::string name;
    tinyobj::material_t property;
    ModelObjRef modelObj;

    gl::Texture2dRef diffuseTexture;
    glm::vec4 diffuseFactor = glm::vec4(0);
    glm::vec3 ambientFactor = glm::vec3(0);
    glm::vec3 specularFactor = glm::vec3(0);
    glm::vec3 emissiveFactor = glm::vec3(0);
    glm::vec3 transmittanceFactor = glm::vec3(0);
    float glossinessFactor = 1.0f;
    float ior = 1.0f;
    int dissolve = 1;
    int illum = 0;

    gl::GlslProgRef ciShader;

    static Ref create(ModelObjRef modelObj, const tinyobj::material_t& property);
    void preDraw();
    void postDraw();
};

struct MeshObj : public nodes::Node3D
{
    typedef std::shared_ptr<MeshObj> Ref;
    tinyobj::shape_t property;
    gl::VboMeshRef vboMesh;
    MaterialObj::Ref material;

    static Ref create(ModelObjRef modelObj, const tinyobj::shape_t& property);

    virtual void draw();
};

struct ModelObj : public MeshObj
{
    static ModelObjRef create(const fs::path& meshPath, std::string* loadingError = nullptr);

    fs::path meshPath;
    fs::path baseDir;

    bool flipV = true;
    vec3 cameraPosition;

    std::vector<MaterialObj::Ref> materials;
    tinyobj::attrib_t attrib;
};
