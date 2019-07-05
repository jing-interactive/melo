#pragma once

#include "Node3D.h"

namespace nodes
{
    Node3DRef loadSceneFromGLTF(const std::string& filename);
    bool writeSceneToGLTF(Node3DRef scene, const std::string& filename);
}
