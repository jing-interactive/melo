#pragma once

#include "syoyo/tiny_obj_loader.h"

#include "Cinder-Nodes/include/Node3D.h"

#include <cinder/gl/gl.h>
#include <memory>
#include <vector>

typedef std::shared_ptr<struct RootObj> RootObjRef;

using namespace ci;

struct MeshObj : public nodes::Node3D
{
    typedef std::shared_ptr<MeshObj> Ref;
    tinyobj::shape_t property;

    static Ref create(RootObjRef rootObj, const tinyobj::shape_t& property);

    void draw();
};

struct MaterialObj
{
    typedef std::shared_ptr<MaterialObj> Ref;
    tinyobj::material_t property;

    gl::Texture2dRef diffuseTexture;
    gl::GlslProgRef ciShader;

    static Ref create(RootObjRef rootObj, const tinyobj::material_t& property);
    void preDraw();
    void postDraw();
};

struct RootObj : public MeshObj
{
    static RootObjRef create(const fs::path& meshPath);

    void update();
    void draw();

    fs::path meshPath;
    fs::path baseDir;

    std::vector<MaterialObj::Ref> materials;
    tinyobj::attrib_t attrib;
};
