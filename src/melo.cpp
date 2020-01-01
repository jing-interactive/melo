#include "melo.h"
#include "cigltf.h"
#include "ciobj.h"
#include "NodeExt.h"
#include "SkyNode.h"
#include <cinder/GeomIo.h>
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
        if (typeName == "SkyNode") return createSkyNode("CathedralRadiance.dds");

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

#define ENTRY(name)  if (meshPath == #name) { auto node = BuiltinMeshNode::create(geom::name()); node->setName(#name); return node;}
        ENTRY(Rect);
        ENTRY(RoundedRect);
        ENTRY(Cube);
        ENTRY(Icosahedron);
        ENTRY(Icosphere);
        ENTRY(Teapot);
        ENTRY(Circle);
        ENTRY(Ring);
        ENTRY(Sphere);
        ENTRY(Capsule);
        ENTRY(Torus);
        ENTRY(TorusKnot);
        ENTRY(Cylinder);
        ENTRY(Plane);
        ENTRY(WireCapsule);
        ENTRY(WireCircle);
        ENTRY(WireRoundedRect);
        ENTRY(WireCube);
        ENTRY(WireCylinder);
        ENTRY(WireCone);
        ENTRY(WireIcosahedron);
        ENTRY(WirePlane);
        ENTRY(WireSphere);
        ENTRY(WireTorus);
#undef ENTRY

        return {};
    }

    NodeRef createGridNode(float meters)
    {
        return GridNode::create(meters);
    }

    NodeRef createSkyNode(const std::string& skyTexturePath)
    {
        return SkyNode::create(skyTexturePath);
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
        AxisAlignedBox localBounds = { node->mBoundBoxMin, node->mBoundBoxMax };
        AxisAlignedBox worldBounds = localBounds.transformed(node->getWorldTransform());

        gl::multModelMatrix(glm::translate(worldBounds.getCenter()) * glm::scale(worldBounds.getSize()));
        mWireCube->draw();
    }

    NodeRef pick(NodeRef parentNode, const ci::CameraPersp& camera, const glm::ivec2& screenPos, uint32_t rayMask)
    {
        // Generate a ray from the camera into our world. Note that we have to
        // flip the vertical coordinate.
        float u = screenPos.x / (float)getWindowWidth();
        float v = screenPos.y / (float)getWindowHeight();
        Ray ray = camera.generateRay(u, 1.0f - v, camera.getAspectRatio());

        auto hit = pick(parentNode, ray, rayMask);

        return hit;
    }

    NodeRef pick(NodeRef parentNode, const ci::Ray& ray, uint32_t rayMask)
    {
        AxisAlignedBox localBounds = { parentNode->mBoundBoxMin, parentNode->mBoundBoxMax };
        AxisAlignedBox worldBounds = localBounds.transformed(parentNode->getWorldTransform());

        // Perform fast detection first - test against the bounding box itself.
        if ((rayMask & parentNode->rayCategory) && !worldBounds.intersects(ray))
        {
            return {};
        }

        //drawBoundingBox(parentNode, Color(0, 1, 1));

        for (const auto& child : parentNode->getChildren())
        {
            if (pick(child, ray, rayMask))
            {
                return child;
            }
        }

        if ((rayMask & parentNode->rayCategory) == 0)
            return {};

        return parentNode;
    }
}
