#pragma once

typedef std::shared_ptr<class ShadowMap> ShadowMapRef;

class ShadowMap {
public:
	static ShadowMapRef create(int size) { return ShadowMapRef(new ShadowMap{ size }); }
	ShadowMap(int size)
	{
		reset(size);
	}

	void reset(int size)
	{
		gl::Texture2d::Format depthFormat;
		depthFormat.setInternalFormat(GL_DEPTH_COMPONENT32F);
		depthFormat.setMagFilter(GL_LINEAR);
		depthFormat.setMinFilter(GL_LINEAR);
		depthFormat.setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		depthFormat.setCompareMode(GL_COMPARE_REF_TO_TEXTURE);
		depthFormat.setCompareFunc(GL_LEQUAL);
		mTextureShadowMap = gl::Texture2d::create(size, size, depthFormat);

		gl::Fbo::Format fboFormat;
		fboFormat.attachment(GL_DEPTH_ATTACHMENT, mTextureShadowMap);
		mShadowMap = gl::Fbo::create(size, size, fboFormat);
	}

	const gl::FboRef& getFbo() const { return mShadowMap; }
	const gl::Texture2dRef& getTexture() const { return mTextureShadowMap; }

	float					getAspectRatio() const { return mShadowMap->getAspectRatio(); }
	ivec2					getSize() const { return mShadowMap->getSize(); }
private:
	gl::FboRef				mShadowMap;
	gl::Texture2dRef		mTextureShadowMap;
};

struct LightData {
	bool						toggleViewpoint;
	float						distanceRadius;
	float						fov;
	CameraPersp					camera;
	vec3						viewpoint;
	vec3						target;
};
