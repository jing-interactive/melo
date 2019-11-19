#include "melo.h"
#include "cigltf.h"
#include "ciobj.h"
#include "NodeExt.h"
#include "SkyNode.h"
#include <cinder/app/App.h>

using namespace ci;
using namespace ci::app;

namespace melo
{
    NodeRef create(const std::string& typeName)
    {
        if (typeName.find(".gltf") != std::string::npos) return createMeshNode(typeName);
        if (typeName.find(".glb") != std::string::npos) return createMeshNode(typeName);
        if (typeName.find(".obj") != std::string::npos) return createMeshNode(typeName);
        if (typeName == "RootNode") return createRootNode();
        if (typeName == "GridNode") return createGridNode();
        if (typeName == "SkyNode") return createSkyNode();

        auto ref = Node::create();
        ref->setName(typeName);

        return ref;
    }

    NodeRef createRootNode()
    {
        auto root = Node::create();
        root->setName("RootNode");

        return root;
    }

    NodeRef createMeshNode(const fs::path& meshPath)
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

    NodeRef createGridNode(float meters)
    {
        return GridNode::create(meters);
    }

    NodeRef createSkyNode()
    {
        return SkyNode::create();
    }
}
