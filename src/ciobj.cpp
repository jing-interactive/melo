#include "ciobj.h"
#include "AssetManager.h"
#include "MiniConfig.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"

using namespace std;

MeshObj::Ref MeshObj::create(RootObjRef rootObj, const tinyobj::shape_t& property)
{
    auto ref = make_shared<MeshObj>();
    ref->property = property;

    return ref;
}

void MeshObj::draw()
{

}

void MaterialObj::preDraw()
{
    ciShader->bind();
    if (diffuseTexture)
        diffuseTexture->bind(0);
}

void MaterialObj::postDraw()
{
    if (diffuseTexture)
        diffuseTexture->unbind(0);
}

MaterialObj::Ref MaterialObj::create(RootObjRef rootObj, const tinyobj::material_t& property)
{
    auto ref = make_shared<MaterialObj>();
    ref->property = property;

    if (!property.diffuse_texname.empty())
    {
        ref->diffuseTexture = am::texture2d(property.diffuse_texname);
        if (!ref->diffuseTexture)
        {
            auto path = rootObj->baseDir / property.diffuse_texname;
            ref->diffuseTexture = am::texture2d(path.string());
        }

        ref->ciShader = am::glslProg("lambert texture");
    }
    else
    {
        ref->ciShader = am::glslProg("lambert");
    }

    return ref;
}


RootObjRef RootObj::create(const fs::path& meshPath)
{
    if (!fs::exists(meshPath))
    {
        CI_LOG_F("File doesn't exist: ") << meshPath;
        return {};
    }

    auto ref = make_shared<RootObj>();
    ref->meshPath = meshPath;
    ref->baseDir = meshPath.parent_path().string();

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(&ref->attrib, &shapes, &materials, &err,
                                meshPath.string().c_str(), ref->baseDir.string().c_str());
    if (!err.empty())
    {
        CI_LOG_E(err);
    }

    if (!ret)
    {
        CI_LOG_E("Failed to load ") << meshPath;
        return {};
    }

    CI_LOG_I("# of vertices ") << (ref->attrib.vertices.size() / 3);
    CI_LOG_I("# of normals  ") << (ref->attrib.normals.size() / 3);
    CI_LOG_I("# of texcoords") << (ref->attrib.texcoords.size() / 2);
    CI_LOG_I("# of materials") << materials.size();
    CI_LOG_I("# of shapes   ") << shapes.size();

    for (auto& item : shapes)
        ref->addChild(MeshObj::create(ref, item));

    // Append `default` material
    materials.push_back(tinyobj::material_t());
    for (auto& item : materials)
        ref->materials.emplace_back(MaterialObj::create(ref, item));

    return ref;
}

void RootObj::update()
{

}

void RootObj::draw()
{

}
