#include "melo.h"
#include "cigltf.h"
#include "ciobj.h"
#include "NodeExt.h"
#include "SkyNode.h"
#include <cinder/app/App.h>

using namespace nodes;
using namespace ci;
using namespace ci::app;

Node3DRef melo::createSceneRoot()
{
    auto root = Node3D::create();
    root->setName("SceneRoot");

    return root;
}

Node3DRef melo::createFromFile(const fs::path& meshPath)
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

Node3DRef melo::createGrid(float meters)
{
    return GridNode::create(meters);
}

Node3DRef melo::createSky()
{
    return SkyNode::create();
}
