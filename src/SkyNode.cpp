#include "SkyNode.h"
#include "AssetManager.h"

using namespace ci;

namespace melo
{
    SkyNode::Ref SkyNode::create(const std::string& skyTexturePath)
    {
        auto ref = std::make_shared<SkyNode>();
        ref->setName("SkyNode");
        ref->mSkyTex = am::textureCubeMap(skyTexturePath);
        auto skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
        if (!skyBoxShader)
        {
            return {};
        }

        ref->skyBoxShader = skyBoxShader;

        ref->mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(1000)), ref->skyBoxShader);

        return ref;

    }

    void SkyNode::predraw(DrawOrder order)
    {
        skyBoxShader->uniform("uCubeMapTex", 0);
        skyBoxShader->uniform("uExposure", 2.0f);
        skyBoxShader->uniform("uGamma", 2.0f);
    }

    void SkyNode::draw(DrawOrder order)
    {
        if (!mSkyBoxBatch && !mSkyTex) return;

        //gl::ScopedDepthWrite depthWrite(false);
        gl::ScopedTextureBind scpTex(mSkyTex, 0);
        mSkyBoxBatch->draw();
    }

}
