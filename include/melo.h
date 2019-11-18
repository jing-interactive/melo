#pragma once

#include "../include/Node3D.h"
#include <cinder/Filesystem.h>

namespace melo
{
    Node3DRef create(const std::string& typeName);

    Node3DRef createRootNode();
    Node3DRef createMeshNode(const cinder::fs::path& meshPath);
    Node3DRef createGridNode(float meters = 100.0f);
    Node3DRef createSkyNode();

    Node3DRef loadScene(const std::string& filename);
    bool writeScene(Node3DRef scene, const std::string& filename);
};
