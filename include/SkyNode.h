#pragma once

#include "Node3D.h"
#include <Cinder/gl/gl.h>

using namespace ci;

struct SkyNode : nodes::Node3D
{
    void setup()
    {
        setName("SkyNode");
        mSkyTex = am::textureCubeMap("skybox.dds");
        skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
        if (!skyBoxShader)
        {
            return;
        }
        skyBoxShader->uniform("uCubeMapTex", 0);
        skyBoxShader->uniform("uExposure", 2.0f);
        skyBoxShader->uniform("uGamma", 2.0f);

        mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(1000)), skyBoxShader);
    }

    void draw()
    {
        if (!mSkyBoxBatch && !mSkyTex) return;

        //gl::ScopedDepthWrite depthWrite(false);
        gl::ScopedTextureBind scpTex(mSkyTex, 0);
        mSkyBoxBatch->draw();
    }

    gl::TextureCubeMapRef mSkyTex;
    gl::BatchRef mSkyBoxBatch;
    gl::GlslProgRef skyBoxShader;
};
