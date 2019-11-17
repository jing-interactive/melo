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
    gl::Texture2dRef normalTexture;

    glm::vec4 diffuseFactor = glm::vec4(1);
    glm::vec3 ambientFactor = glm::vec3(1);
    glm::vec3 specularFactor = glm::vec3(1);
    glm::vec3 emissiveFactor = glm::vec3(1);
    glm::vec3 transmittanceFactor = glm::vec3(0);
    float glossinessFactor = 1.0f;
    float ior = 1.0f;
    int dissolve = 1;
    int illum = 0;

    gl::GlslProgRef ciShader;

    melo::MaterialType materialType;

    static Ref create(ModelObjRef modelObj, const tinyobj::material_t& property);
    void predraw();
    void postdraw();
};

struct MeshObj : public melo::Node3D
{
    typedef std::shared_ptr<MeshObj> Ref;
    tinyobj::shape_t property;
    
    struct SubMesh
    {
        // very stupid design of OBJ spec...
        gl::VboMeshRef vboMesh;
        MaterialObj::Ref material;
        glm::vec3 boundBoxMin, boundBoxMax;

        // these are scratch data, keep them here for easier life...
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<Color> colors;
        std::vector<uint32_t> indexArray;

        void setup();
        void draw();
    };

    std::unordered_map<int, SubMesh> submeshes;

    static Ref create(ModelObjRef modelObj, const tinyobj::shape_t& property);

    virtual void draw();
    virtual void predraw();
    virtual void postdraw();
};

struct ModelObj : public MeshObj
{
    static ModelObjRef create(const fs::path& meshPath, std::string* loadingError = nullptr);

    fs::path meshPath;
    fs::path baseDir;

    bool flipV = true;

    std::vector<MaterialObj::Ref> materials;
    tinyobj::attrib_t attrib;
};
