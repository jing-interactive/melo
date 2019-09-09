/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Node.h"
#include <glm/gtc/quaternion.hpp>

namespace nodes {

    // Basic support for 3D nodes
    typedef std::shared_ptr<class Node3D> Node3DRef;

    class Node3D : public Node {
    public:
        Node3D(void);
        virtual ~Node3D(void);

        static Node3DRef create();

        // getters and setters
        virtual glm::vec3 getPosition() const { return mPosition; }

        virtual void setPosition(const glm::vec3 &pt)
        {
            mPosition = pt;
            invalidateTransform();
        }

        virtual glm::quat getRotation() const { return mRotation; }
        virtual void setRotation(float radians);
        virtual void setRotation(const glm::vec3 &radians);
        virtual void setRotation(const glm::vec3 &axis, float radians);
        virtual void setRotation(const glm::quat &rot);

        virtual glm::vec3 getScale() const { return mScale; }

        virtual void setScale(const glm::vec3 &scale)
        {
            mScale = scale;
            invalidateTransform();
        }

        virtual glm::vec3 getAnchor() const { return mAnchor; }

        virtual void setAnchor(const glm::vec3 &pt)
        {
            mAnchor = pt;
            invalidateTransform();
        }

        void setConstantTransform(const glm::mat4& transform);

        // stream support
        virtual inline std::string toString() const { return "Node3D"; }

        glm::vec3 mBoundBoxMin, mBoundBoxMax;
        glm::vec3 cameraPosition = { 1,1,1 };

    protected:
        glm::vec3 mPosition;
        glm::quat mRotation;
        glm::vec3 mScale;
        glm::vec3 mAnchor;

        // required function (see: class Node)
        virtual void transform() const;

        // whether to ingore values of mPosition, mRotation, mScale, mAnchor
        glm::mat4 mConstantTransform;
        bool mIsConstantTransform;
    };
} // namespace nodes