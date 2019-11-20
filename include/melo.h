#pragma once

#include "../include/Node.h"
#include <cinder/Filesystem.h>
#include <cinder/Camera.h>

namespace melo
{
    NodeRef create(const std::string& typeName);

    NodeRef createRootNode();
    NodeRef createMeshNode(const cinder::fs::path& meshPath);
    NodeRef createGridNode(float meters = 100.0f);
    NodeRef createSkyNode();

    NodeRef loadScene(const std::string& filename);
    bool writeScene(NodeRef scene, const std::string& filename);

    void drawBoundingBox(NodeRef node, const ci::Color& color = { 1, 1, 0 });

    NodeRef pick(NodeRef parentNode, const ci::CameraPersp& camera, const glm::ivec2& screenPos, uint32_t rayMask = 0xFFFFFF);
    NodeRef pick(NodeRef parentNode, const ci::Ray& ray, uint32_t rayMask = 0xFFFFFF);
};
