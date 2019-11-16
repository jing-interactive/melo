#include "SkyNode.h"
#include "AssetManager.h"

using namespace ci;

SkyNode::Ref SkyNode::create()
{
    auto ref = std::make_shared<SkyNode>();
    ref->setName("SkyNode");
    ref->mSkyTex = am::textureCubeMap("skybox.dds");
    auto skyBoxShader = am::glslProg("SkyBox.vert", "SkyBox.frag");
    if (!ref->skyBoxShader)
    {
        return {};
    }
    skyBoxShader->uniform("uCubeMapTex", 0);
    skyBoxShader->uniform("uExposure", 2.0f);
    skyBoxShader->uniform("uGamma", 2.0f);
    ref->skyBoxShader = skyBoxShader;

    ref->mSkyBoxBatch = gl::Batch::create(geom::Cube().size(vec3(1000)), ref->skyBoxShader);

    return ref;

}

void SkyNode::draw()
{
    if (!mSkyBoxBatch && !mSkyTex) return;

    //gl::ScopedDepthWrite depthWrite(false);
    gl::ScopedTextureBind scpTex(mSkyTex, 0);
    mSkyBoxBatch->draw();
}
