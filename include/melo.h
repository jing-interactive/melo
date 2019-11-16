#pragma once

#include "../include/Node3D.h"
#include <cinder/Filesystem.h>

namespace melo
{
    nodes::Node3DRef createSceneRoot();
    nodes::Node3DRef createFromFile(const cinder::fs::path& meshPath);
    nodes::Node3DRef createGrid(float meters = 100.0f);
    nodes::Node3DRef createSky();
};
