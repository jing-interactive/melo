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

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#ifndef CINDER_LESS
// forward declarations
namespace cinder
{
    class AxisAlignedBox;
    class Camera;
    class CameraPersp;
    class CameraStereo;
    class CameraOrtho;
    namespace app
    {
        class MouseEvent;
        class KeyEvent;
        class ResizeEvent;
        class FileDropEvent;
    } // namespace app
} // namespace cinder
#include "cinder/gl/GlslProg.h"
#endif

namespace nodes
{

    typedef std::shared_ptr<class Node> NodeRef;
    typedef std::shared_ptr<const class Node> NodeConstRef;
    typedef std::weak_ptr<class Node> NodeWeakRef;
    typedef std::vector<NodeRef> NodeList;
    typedef std::map<unsigned int, NodeWeakRef> NodeMap;

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
        //! returns a node higher up in the hierarchy of the desired type, if any
        template <class T> std::shared_ptr<T> getTreeParent() const
        {
            std::shared_ptr<T> node = std::dynamic_pointer_cast<T>(mParent.lock());
            if (node)
                return node;
            else if (mParent.lock())
                return mParent.lock()->getTreeParent<T>();
            else
                return node;
        }

        // functions to get the Node's unique identifier and to quickly find a Node with a specific
        // uuid
        unsigned int getUuid() const { return mUuid; }
        glm::vec3 getUuidColor() const { return uuidToColor(mUuid); }

        static glm::vec3 uuidToColor(unsigned int uuid)
        {
            return glm::vec3((uuid & 0xFF) / 255.0f, ((uuid >> 8) & 0xFF) / 255.0f,
                             ((uuid >> 16) & 0xFF) / 255.0f);
        }
        static unsigned int colorToUuid(glm::vec3 color)
        {
            return colorToUuid((unsigned char)(color.r * 255), (unsigned char)(color.g * 255),
                               (unsigned char)(color.b * 255));
        }
        static unsigned int colorToUuid(unsigned char r, unsigned char g, unsigned char b)
        {
            return r + (g << 8) + (b << 16);
        }

        static NodeRef findNode(unsigned int uuid) { return uuidLookup[uuid].lock(); }

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

        //!
        NodeRef findChild(unsigned int uuid);

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

        //!
        virtual void setClickable(bool clickable = true) { mIsClickable = clickable; }
        //! returns wether this node is clickable
        virtual bool isClickable() const { return mIsClickable; }

        //! returns the transformation matrix of this node
        const glm::mat4& getTransform() const
        {
            if (mIsTransformInvalidated) transform();
            return mTransform;
        }
        //! sets the transformation matrices of this node
        void setTransform(const glm::mat4& transform) const;
        //! returns the accumulated transformation matrix of this node
        const glm::mat4& getWorldTransform() const
        {
            if (mIsTransformInvalidated) transform();
            return mWorldTransform;
        }
        //!
        void invalidateTransform() const
        {
            mIsTransformInvalidated = true;

            for (auto& child : mChildren)
                child->invalidateTransform();
        }

        //!
        virtual void setSelected(bool selected = true) { mIsSelected = selected; }
        //! returns wether this node is selected
        virtual bool isSelected() const { return mIsSelected; }

        //! signal parent that this node has been clicked or activated
        virtual void selectChild(NodeRef node)
        {
            for (auto& child : mChildren)
                child->setSelected(child == node);
        }
        //! signal parent that this node has been released or deactivated
        virtual void deselectChild(NodeRef node)
        {
            for (auto& child : mChildren)
                child->setSelected(false);
        }

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

#ifndef CINDER_LESS
        // supported events
        //! calls the mouseMove() function of this node and all its decendants until a TRUE is
        //! passed back
        bool treeMouseMove(ci::app::MouseEvent event);
        //! calls the mouseDown() function of this node and all its decendants until a TRUE is
        //! passed back
        bool treeMouseDown(ci::app::MouseEvent event);
        //! calls the mouseDrag() function of this node and all its decendants until a TRUE is
        //! passed back
        bool treeMouseDrag(ci::app::MouseEvent event);
        //! calls the mouseUp() function of this node and all its decendants until a TRUE is passed
        //! back
        bool treeMouseUp(ci::app::MouseEvent event);

        virtual bool mouseMove(ci::app::MouseEvent event);
        virtual bool mouseDown(ci::app::MouseEvent event);
        virtual bool mouseDrag(ci::app::MouseEvent event);
        virtual bool mouseUp(ci::app::MouseEvent event);

        // support for easy picking system
        virtual bool mouseUpOutside(ci::app::MouseEvent event);

        //! calls the keyDown() function of this node and all its decendants until a TRUE is passed
        //! back
        bool treeKeyDown(ci::app::KeyEvent event);
        //! calls the keyUp() function of this node and all its decendants until a TRUE is passed
        //! back
        bool treeKeyUp(ci::app::KeyEvent event);

        virtual bool keyDown(ci::app::KeyEvent event);
        virtual bool keyUp(ci::app::KeyEvent event);
#endif

        //! calls the resize() function of this node and all its decendants until a TRUE is passed
        //! back
        bool treeResize();

        virtual bool resize();

        // stream support
        virtual inline std::string toString() const { return "Node"; }

        void setName(const std::string& name);
        const std::string& getName() const;

      protected:
        std::string mName;

        bool mIsVisible;
        bool mIsClickable;
        bool mIsSelected;

        const unsigned int mUuid;

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
        //! uuidCount is used to generate new unique id's
        static unsigned int uuidCount;
        //! uuidLookup allows us to quickly find a Node by id
        static NodeMap uuidLookup;

        mutable bool mIsTransformInvalidated;
        mutable glm::mat4 mTransform;
        mutable glm::mat4 mWorldTransform;
    };

} // namespace nodes