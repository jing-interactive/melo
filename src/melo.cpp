#include "melo.h"
#include "cigltf.h"
#include "ciobj.h"
#include "NodeExt.h"
#include "SkyNode.h"
#include <cinder/app/App.h>
#include <glm/gtx/transform.hpp>

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

    void drawBoundingBox(NodeRef node, const ci::Color& color)
    {
        static gl::BatchRef		mWireCube;
        if (!mWireCube)
        {
            auto colorShader = gl::getStockShader(gl::ShaderDef().color());
            mWireCube = gl::Batch::create(geom::WireCube(), colorShader);
        }
        gl::ScopedColor clr(color);
        gl::ScopedModelMatrix model;
        AxisAlignedBox bounds = { node->mBoundBoxMin, node->mBoundBoxMax };
        gl::multModelMatrix(glm::translate(bounds.getCenter()) * glm::scale(bounds.getSize()));
        mWireCube->draw();
    }

    NodeRef pick(NodeRef parentNode, const ci::CameraPersp& camera, const glm::ivec2& screenPos)
    {
        // Generate a ray from the camera into our world. Note that we have to
        // flip the vertical coordinate.
        float u = screenPos.x / (float)getWindowWidth();
        float v = screenPos.y / (float)getWindowHeight();
        Ray ray = camera.generateRay(u, 1.0f - v, camera.getAspectRatio());

        auto hit = pick(parentNode, ray);

        return hit;
    }

    NodeRef pick(NodeRef parentNode, const ci::Ray& ray)
    {
        AxisAlignedBox localBounds = { parentNode->mBoundBoxMin, parentNode->mBoundBoxMax };
        AxisAlignedBox worldBoundsApprox = localBounds.transformed(parentNode->getWorldTransform());

        // Draw the approximated bounding box in cyan.

        // Perform fast detection first - test against the bounding box itself.
        if (!worldBoundsApprox.intersects(ray))
            return {};

        drawBoundingBox(parentNode, Color(0, 1, 1));

        for (const auto& child : parentNode->getChildren())
        {
            if (pick(child, ray))
            {
                return child;
            }
        }

        return parentNode;
    }
}
