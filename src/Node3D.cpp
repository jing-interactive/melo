/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted
 provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions
 and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 and the following disclaimer in the documentation and/or other materials provided with the
 distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CINDER_LESS
#include <cinder/gl/Texture.h>
#endif
#include "../include/Node3D.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

using namespace std;

namespace melo
{
#ifndef CINDER_LESS
    ci::gl::TextureCubeMapRef Node3D::radianceTexture;
    ci::gl::TextureCubeMapRef Node3D::irradianceTexture;
    ci::gl::Texture2dRef Node3D::brdfLUTTexture;
#endif
    Node3D::Node3D(void) : mScale(1), mIsConstantTransform(false) { setName("Node3D"); }

    Node3DRef Node3D::create()
    {
        return make_shared<Node3D>();
    }

    Node3D::~Node3D(void) {}

    void Node3D::setRotation(float radians)
    {
        mRotation = glm::angleAxis(radians, glm::vec3(0, 1, 0));
        invalidateTransform();
    }
    void Node3D::setRotation(const glm::vec3& radians)
    {
        mRotation = glm::rotation(glm::vec3(0), radians);
        invalidateTransform();
    }
    void Node3D::setRotation(const glm::vec3& axis, float radians)
    {
        mRotation = glm::angleAxis(radians, axis);
        invalidateTransform();
    }
    void Node3D::setRotation(const glm::quat& rot)
    {
        mRotation = rot;
        invalidateTransform();
    }
    void Node3D::transform() const
    {
        if (mIsConstantTransform)
        {
            setTransform(mConstantTransform);
        }
        else
        {
            // construct transformation matrix
            glm::mat4 transform = glm::translate(mPosition);
            transform *= glm::toMat4(mRotation);
            transform *= glm::scale(mScale);
            transform *= glm::translate(-mAnchor);
            setTransform(transform);
        }
    }
    void Node3D::setConstantTransform(const glm::mat4& transform)
    {
        mIsConstantTransform = true;
        mConstantTransform = transform;
    }
} // namespace melo