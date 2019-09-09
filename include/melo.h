#pragma once

#include "../include/Node3D.h"
#include <cinder/Filesystem.h>

struct Melo
{
    static nodes::Node3DRef createSceneRoot();
    static nodes::Node3DRef createFromFile(const cinder::fs::path& meshPath);
    static nodes::Node3DRef createGrid(float meters = 100.0f);
};
