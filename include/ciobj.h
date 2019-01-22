#pragma once

#include "syoyo/tiny_obj_loader.h"

#include "Cinder-Nodes/include/Node3D.h"

#include <cinder/gl/gl.h>
#include <memory>
#include <vector>

typedef std::shared_ptr<struct RootObj> RootObjRef;

using namespace ci;

struct MaterialObj
{
    typedef std::shared_ptr<MaterialObj> Ref;
    tinyobj::material_t property;
    RootObjRef rootObj;

    gl::Texture2dRef diffuseTexture;
    vec4 diffuseFactor = { 1, 1, 1, 1 };

    gl::GlslProgRef ciShader;

    static Ref create(RootObjRef rootObj, const tinyobj::material_t& property);
    void preDraw();
    void postDraw();
};

struct MeshObj : public nodes::Node3D
{
    typedef std::shared_ptr<MeshObj> Ref;
    tinyobj::shape_t property;
    gl::VboMeshRef vboMesh;
    MaterialObj::Ref material;

    static Ref create(RootObjRef rootObj, const tinyobj::shape_t& property);

    virtual void draw();
};

struct RootObj : public MeshObj
{
    static RootObjRef create(const fs::path& meshPath);

    fs::path meshPath;
    fs::path baseDir;

    bool flipV = true;
    vec3 cameraPosition;

    std::vector<MaterialObj::Ref> materials;
    tinyobj::attrib_t attrib;
};
