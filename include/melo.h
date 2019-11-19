#pragma once

#include "../include/Node.h"
#include <cinder/Filesystem.h>

namespace melo
{
    NodeRef create(const std::string& typeName);

    NodeRef createRootNode();
    NodeRef createMeshNode(const cinder::fs::path& meshPath);
    NodeRef createGridNode(float meters = 100.0f);
    NodeRef createSkyNode();

    NodeRef loadScene(const std::string& filename);
    bool writeScene(NodeRef scene, const std::string& filename);
};
