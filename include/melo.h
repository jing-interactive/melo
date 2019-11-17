#pragma once

#include "../include/Node3D.h"
#include <cinder/Filesystem.h>

namespace melo
{
    Node3DRef create(const std::string& typeName);

    Node3DRef createRootNode();
    Node3DRef createGltfNode(const cinder::fs::path& meshPath);
    Node3DRef createGridNode(float meters = 100.0f);
    Node3DRef createSkyNode();
};
