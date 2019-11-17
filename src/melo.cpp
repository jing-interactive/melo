#include "melo.h"
#include "cigltf.h"
#include "ciobj.h"
#include "NodeExt.h"
#include "SkyNode.h"
#include <cinder/app/App.h>

using namespace melo;
using namespace ci;
using namespace ci::app;

Node3DRef melo::create(const std::string& typeName)
{
    if (typeName == "RootNode") return melo::createRootNode();
    if (typeName.find(".gltf") != std::string::npos) return melo::createGltfNode(typeName);
    if (typeName == "GridNode") return melo::createGridNode();
    if (typeName == "SkyNode") return melo::createSkyNode();

    auto ref = melo::Node3D::create();
    ref->setName(typeName);

    return ref;
}

Node3DRef melo::createRootNode()
{
    auto root = Node3D::create();
    root->setName("RootNode");

    return root;
}

Node3DRef melo::createGltfNode(const fs::path& meshPath)
{
    fs::path realPath = meshPath;
    if (!fs::exists(realPath))
    {
        realPath = getAssetPath(realPath);
        if (!fs::exists(realPath)) return {};
    }

    if (realPath.extension() == ".obj")
        return ModelObj::create(realPath);

    if (realPath.extension() == ".gltf" || realPath.extension() == ".glb")
        return ModelGLTF::create(realPath);

    return {};
}

Node3DRef melo::createGridNode(float meters)
{
    return GridNode::create(meters);
}

Node3DRef melo::createSkyNode()
{
    return SkyNode::create();
}
