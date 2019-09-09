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

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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
} // namespace cinder
#include "cinder/gl/GlslProg.h"
#include "cinder/Signals.h"
#endif

namespace nodes
{
    typedef std::shared_ptr<class Node> NodeRef;
    typedef std::shared_ptr<const class Node> NodeConstRef;
    typedef std::weak_ptr<class Node> NodeWeakRef;
    typedef std::vector<NodeRef> NodeList;

    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        Node(void);
        virtual ~Node(void);

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
        //! puts a specific child on top of all other children of this node
        void putOnTop(NodeRef node);
        //! returns wether a specific child is on top of all other children
        bool isOnTop(NodeConstRef node) const;
        //! puts a specific child below all other children of this node
        void moveToBottom(NodeRef node);

        //! returns a list of all children of the specified type
        template <class T> std::vector<std::shared_ptr<T>> getChildren()
        {
            std::vector<std::shared_ptr<T>> result;
            for (auto& child : mChildren)
            {
                std::shared_ptr<T> node = std::dynamic_pointer_cast<T>(child);
                if (node) result.push_back(node);
            }
            return result;
        }

        // child functions
        //! removes this node from its parent
        void removeFromParent();
        //! puts this node on top of all its siblings
        void putOnTop();
        //! returns wether this node is on top of all its siblings
        bool isOnTop() const;
        //! puts this node below all its siblings
        void moveToBottom();

        //! enables or disables visibility of this node (invisible nodes are not drawn and can not
        //! receive events, but they still receive updates)
        virtual void setVisible(bool visible = true) { mIsVisible = visible; }
        //! returns wether this node is visible
        virtual bool isVisible() const { return mIsVisible; }
        //!
        virtual bool toggleVisible()
        {
            setVisible(!mIsVisible);
            return mIsVisible;
        }

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

        // stream support
        virtual inline std::string toString() const { return "Node"; }

        void setName(const std::string& name);
        const std::string& getName() const;

    protected:
        std::string mName;

        bool mIsVisible;

        NodeWeakRef mParent;
        NodeList mChildren;

    protected:
        virtual void setup() {}
        virtual void shutdown() {}
        virtual void update(double elapsed = 0.0) {}
        virtual void draw() {}

        //! function that is called right before drawing this node
        virtual void predraw() {}
        //! function that is called right after drawing this node
        virtual void postdraw() {}

        //! required transform() function to populate the transform matrix
        virtual void transform() const = 0;

    private:
        bool mIsSetup;

        //! nodeCount is used to count the number of Node instances for debugging purposes
        static int nodeCount;

        mutable bool mIsTransformInvalidated;
        mutable glm::mat4 mTransform;
        mutable glm::mat4 mWorldTransform;
    };

} // namespace nodes