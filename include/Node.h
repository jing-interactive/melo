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

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <string>

#ifndef CINDER_LESS
 // forward declarations
namespace cinder
{
    class AxisAlignedBox;
    namespace app
    {
        class MouseEvent;
        class KeyEvent;
    } // namespace app
    namespace gl {

        typedef std::shared_ptr<class Texture2d>		Texture2dRef;
        typedef std::shared_ptr<class TextureCubeMap>	TextureCubeMapRef;
    }
} // namespace cinder
#include "cinder/gl/GlslProg.h"
#include "cinder/Signals.h"
#else
#define GLM_FORCE_CTOR_INIT
#endif


#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace melo
{
    typedef std::shared_ptr<class Node> NodeRef;
    typedef std::shared_ptr<const class Node> NodeConstRef;
    typedef std::weak_ptr<class Node> NodeWeakRef;
    typedef std::vector<NodeRef> NodeList;

    enum MaterialType
    {
        MATERIAL_PBR_METAL_ROUGHNESS,
        MATERIAL_PBR_SPEC_GLOSSINESS,
        MATERIAL_UNLIT,

        MATERIAL_COUNT,
    };

    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        Node();
        virtual ~Node();

        //! sets the node's parent node (using weak reference to avoid objects not getting
        //! destroyed)
        void setParent(NodeRef node) { mParent = NodeWeakRef(node); }
        //! returns the node's parent node
        NodeRef getParent() const { return mParent.lock(); }
        //! returns the node's parent node (provide a templated function for easier down-casting of
        //! nodes)
        template <class T> std::shared_ptr<T> getParent() const
        {
            return std::dynamic_pointer_cast<T>(mParent.lock());
        }

        // parent functions
        //! returns wether this node has a specific child
        bool hasChild(NodeRef node) const;
        //! adds a child to this node if it wasn't already a child of this node
        void addChild(NodeRef node);
        //! removes a specific child from this node
        void removeChild(NodeRef node);
        //! removes all children of this node
        void removeChildren();

        NodeList& getChildren()
        {
            return mChildren;
        }

        // child functions
        //! removes this node from its parent
        void removeFromParent();

        //! enables or disables visibility of this node (invisible nodes are not drawn and can not
        //! receive events, but they still receive updates)
        virtual void setVisible(bool visible = true) { mIsVisible = visible; }
        //! returns wether this node is visible
        virtual bool isVisible() const { return mIsVisible; }

        //! returns the transformation matrix of this node
        const glm::mat4& getTransform() const;
        //! sets the transformation matrices of this node
        void setTransform(const glm::mat4& transform) const;
        //! returns the accumulated transformation matrix of this node
        const glm::mat4& getWorldTransform() const;
        //!
        void invalidateTransform() const;

        // tree parse functions
        void treeVisitor(std::function<void(NodeRef)> visitor);
        //! calls the setup() function of this node and all its decendants
        void treeSetup();
        //! calls the shutdown() function of this node and all its decendants
        void treeShutdown();
        //! calls the update() function of this node and all its decendants
        void treeUpdate(double elapsed = 0.0);
        //! calls the draw() function of this node and all its decendants
        void treeDraw();

        void setName(const std::string& name);
        const std::string& getName() const;

    protected:
        std::string mName;

        bool mIsVisible;

        NodeWeakRef mParent;
        NodeList mChildren;

        glm::vec3 mPosition;
        glm::quat mRotation;
        glm::vec3 mScale;
        glm::vec3 mAnchor;

        // whether to ingore values of mPosition, mRotation, mScale, mAnchor
        glm::mat4 mConstantTransform;
        bool mIsConstantTransform;

    protected:
        virtual void setup() {}
        virtual void shutdown() {}
        virtual void update(double elapsed = 0.0) {}
        virtual void draw() {}

        //! function that is called right before drawing this node
        virtual void predraw() {}
        //! function that is called right after drawing this node
        virtual void postdraw() {}

        // required function (see: class Node)
        virtual void transform() const;
    private:
        bool mIsSetup;

        mutable bool mIsTransformInvalidated;
        mutable glm::mat4 mTransform;
        mutable glm::mat4 mWorldTransform;

    public:
        static NodeRef create();

        // getters and setters
        virtual glm::vec3 getPosition() const { return mPosition; }

        virtual void setPosition(const glm::vec3& pt)
        {
            mPosition = pt;
            invalidateTransform();
        }

        virtual glm::quat getRotation() const { return mRotation; }
        virtual void setRotation(float radians);
        virtual void setRotation(const glm::vec3& radians);
        virtual void setRotation(const glm::vec3& axis, float radians);
        virtual void setRotation(const glm::quat& rot);

        virtual glm::vec3 getScale() const { return mScale; }

        virtual void setScale(const glm::vec3& scale)
        {
            mScale = scale;
            invalidateTransform();
        }

        virtual glm::vec3 getAnchor() const { return mAnchor; }

        virtual void setAnchor(const glm::vec3& pt)
        {
            mAnchor = pt;
            invalidateTransform();
        }

        void setConstantTransform(const glm::mat4& transform);

        glm::vec3 mBoundBoxMin, mBoundBoxMax;
        glm::vec3 cameraPosition = { 1,1,1 };

#ifndef CINDER_LESS
        static ci::gl::TextureCubeMapRef radianceTexture;
        static ci::gl::TextureCubeMapRef irradianceTexture;
        static ci::gl::Texture2dRef brdfLUTTexture;
#endif

    };

} // namespace melo