#pragma once

#include "Node3D.h"
#include <Cinder/gl/gl.h>

struct SkyNode : nodes::Node3D
{
    typedef std::shared_ptr<SkyNode> Ref;

    static Ref create();

    void predraw() override;
    void draw() override;

    ci::gl::TextureCubeMapRef mSkyTex;
    ci::gl::BatchRef mSkyBoxBatch;
    ci::gl::GlslProgRef skyBoxShader;
};
