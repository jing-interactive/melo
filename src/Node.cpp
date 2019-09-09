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

#include "../include/Node.h"

#ifndef CINDER_LESS
#include "cinder/app/App.h"

using namespace ci;
using namespace ci::app;

#endif
using namespace std;

namespace nodes
{

    int Node::nodeCount = 0;

    Node::Node(void)
        : mIsVisible(true),
        mIsSetup(false), mIsTransformInvalidated(true)
    {
        // default constructor for [Node]
        nodeCount++;
    }

    Node::~Node(void)
    {
        // remove all children safely
        removeChildren();

        //
        nodeCount--;
    }

    void Node::removeFromParent()
    {
        NodeRef node = mParent.lock();
        if (node)
            node->removeChild(shared_from_this());
    }

    void Node::addChild(NodeRef node)
    {
        if (node && !hasChild(node))
        {
            // remove child from current parent
            NodeRef parent = node->getParent();
            if (parent)
                parent->removeChild(node);

            // add to children
            mChildren.push_back(node);

            // set parent
            node->setParent(shared_from_this());
        }
    }

    void Node::removeChild(NodeRef node)
    {
        auto itr = std::find(mChildren.begin(), mChildren.end(), node);
        if (itr != mChildren.end())
        {
            // reset parent
            (*itr)->setParent(NodeRef());

            // remove from children
            mChildren.erase(itr);
        }
    }

    void Node::removeChildren()
    {
        for (auto itr = mChildren.begin(); itr != mChildren.end();)
        {
            // reset parent
            (*itr)->setParent(NodeRef());

            // remove from children
            itr = mChildren.erase(itr);
        }
    }

    bool Node::hasChild(NodeRef node) const
    {
        auto itr = std::find(mChildren.begin(), mChildren.end(), node);
        return (itr != mChildren.end());
    }

    void Node::putOnTop()
    {
        NodeRef parent = getParent();
        if (parent)
            parent->putOnTop(shared_from_this());
    }

    void Node::putOnTop(NodeRef node)
    {
        // remove from list
        auto itr = std::find(mChildren.begin(), mChildren.end(), node);
        if (itr == mChildren.end())
            return;

        mChildren.erase(itr);

        // add to end of list
        mChildren.push_back(node);
    }

    bool Node::isOnTop() const
    {
        NodeRef parent = getParent();
        if (parent)
            return parent->isOnTop(shared_from_this());
        else
            return false;
    }

    bool Node::isOnTop(NodeConstRef node) const
    {
        if (mChildren.empty())
            return false;
        if (mChildren.back() == node)
            return true;
        return false;
    }

    void Node::moveToBottom()
    {
        NodeRef parent = getParent();
        if (parent)
            parent->moveToBottom(shared_from_this());
    }

    //! sets the transformation matrix of this node

    void Node::setTransform(const glm::mat4& transform) const
    {
        mTransform = transform;

        auto parent = getParent();
        if (parent)
            mWorldTransform = parent->getWorldTransform() * mTransform;
        else
            mWorldTransform = mTransform;

        mIsTransformInvalidated = false;

        for (auto& child : mChildren)
            child->invalidateTransform();
    }

    void Node::moveToBottom(NodeRef node)
    {
        // remove from list
        auto itr = std::find(mChildren.begin(), mChildren.end(), node);
        if (itr == mChildren.end())
            return;

        mChildren.erase(itr);

        // add to start of list
        mChildren.insert(mChildren.begin(), node);
    }

    void Node::treeVisitor(std::function<void(NodeRef)> visitor)
    {
        visitor(shared_from_this());
        for (auto& child : mChildren)
            child->treeVisitor(visitor);
    }

    void Node::treeSetup()
    {
        setup();

        for (auto& node : mChildren)
            node->treeSetup();
    }

    void Node::treeShutdown()
    {
        for (auto itr = mChildren.rbegin(); itr != mChildren.rend(); ++itr)
            (*itr)->treeShutdown();

        shutdown();
    }

    void Node::treeUpdate(double elapsed)
    {
        // let derived class perform animation
        update(elapsed);

        if (!mIsSetup)
        {
            setup();
            mIsSetup = true;
        }

        // update this node's children
        for (auto& node : mChildren)
            node->treeUpdate(elapsed);
    }

    void Node::treeDraw()
    {
        if (!mIsVisible)
            return;

        if (!mIsSetup)
        {
            setup();
            mIsSetup = true;
        }

        // update transform matrix by calling derived class's function
        if (mIsTransformInvalidated)
            transform();

        // let derived class know we are about to draw stuff
#if defined(CINDER_MSW_DESKTOP)
        if (!mName.empty())
            gl::pushDebugGroup(mName);
#endif
        predraw();

#ifndef CINDER_LESS
        // apply transform
        gl::pushModelView();

        // usual way to update model matrix
        gl::setModelMatrix(getWorldTransform());

        // draw this node by calling derived class
        draw();

        // draw this node's children
        for (auto& child : mChildren)
            child->treeDraw();

        // restore transform
        gl::popModelView();
#endif

        postdraw();
#if defined(CINDER_MSW_DESKTOP)
        if (!mName.empty())
            gl::popDebugGroup();
#endif
    }

    void Node::setName(const string& name) { mName = name; }

    const string& Node::getName() const { return mName; }

    const glm::mat4& Node::getTransform() const
    {
        if (mIsTransformInvalidated) transform();
        return mTransform;
    }

    const glm::mat4& Node::getWorldTransform() const
    {
        if (mIsTransformInvalidated) transform();
        return mWorldTransform;
    }

    void Node::invalidateTransform() const
    {
        mIsTransformInvalidated = true;

        for (auto& child : mChildren)
            child->invalidateTransform();
    }

} // namespace nodes
