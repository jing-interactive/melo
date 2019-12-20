#pragma once

#include "Node.h"
#include <Cinder/gl/gl.h>

namespace melo
{
    struct SkyNode : Node
    {
        typedef std::shared_ptr<SkyNode> Ref;

        static Ref create(const std::string& skyTexturePath);

        void predraw() override;
        void draw() override;

        ci::gl::TextureCubeMapRef mSkyTex;
        ci::gl::BatchRef mSkyBoxBatch;
        ci::gl::GlslProgRef skyBoxShader;
    };
}