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

#include "Node2D.h"

#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"
#include "cinder/Function.h"

//! For convenience, create a new type for the shared pointer and node list
typedef std::shared_ptr<class NodeRectangle>	NodeRectangleRef;
typedef std::vector<NodeRectangleRef>			NodeRectangleList;

//! Our class extends a simple 2D node
class NodeRectangle : public nodes::Node2D
{
public:
    //TODO: Add CENTER MODE
    enum TouchMode
    {
        UNTOUCHED,
        DRAGGING,
        RESIZING
    };

    NodeRectangle(void);
    virtual ~NodeRectangle(void);

    //! The nodes support Cinder's game loop methods:
    //! setup(), shutdown(), update(), draw().
    //! Note that the update() method takes a double,
    //! which is the elapsed time since last update in seconds.
    void setup();
    void update(double elapsed = 0.0);
    void draw();

    void setTextures(ci::gl::TextureRef texPressed = nullptr, ci::gl::TextureRef texNormal = nullptr, ci::gl::TextureRef texDisabled = nullptr);

    //! The nodes support Cinder's event methods:
    //! mouseMove(), mouseDown(), mouseDrag(), mouseUp(), keyDown(), keyUp() and resize()
    bool mouseMove(ci::app::MouseEvent event);
    bool mouseDown(ci::app::MouseEvent event);
    bool mouseDrag(ci::app::MouseEvent event);
    bool mouseUp(ci::app::MouseEvent event);

    void setMouseUp(std::function<void(ci::app::MouseEvent)> callback) { mUpCallback = callback; }
    template<typename T>
    void setMouseUp(T *obj, void (T::*callback)(ci::app::MouseEvent)) { mUpCallback = std::bind1st(std::mem_fun(callback), obj); }

    void setMouseDown(std::function<void(ci::app::MouseEvent)> callback) { mDownCallback = callback; }
    template<typename T>
    void setMouseDown(T *obj, void (T::*callback)(ci::app::MouseEvent)) { mDownCallback = std::bind1st(std::mem_fun(callback), obj); }

    void setMouseMove(std::function<void(ci::app::MouseEvent)> callback) { mMoveCallback = callback; }
    template<typename T>
    void setMouseMove(T *obj, void (T::*callback)(ci::app::MouseEvent)) { mMoveCallback = std::bind1st(std::mem_fun(callback), obj); }

    void setMouseDrag(std::function<void(ci::app::MouseEvent)> callback) { mDragCallback = callback; }
    template<typename T>
    void setMouseDrag(T *obj, void (T::*callback)(ci::app::MouseEvent)) { mDragCallback = std::bind1st(std::mem_fun(callback), obj); }

    bool mCanvasMode;
    bool mAlwaysHighlit;
    void* mUserData;
protected:
    TouchMode		mTouchMode;

    bool mIsClickable;
    bool mIsSelected;

	ci::vec2 mInitialPosition;
	ci::quat mInitialRotation;
	ci::vec2 mInitialScale;

	ci::vec2 mInitialMouse;
	ci::vec2 mCurrentMouse;

    // TODO: merge
    ci::gl::TextureRef mTexPressed;
    ci::gl::TextureRef mTexNormal;
    ci::gl::TextureRef mTexDisabled;

    std::function<void(ci::app::MouseEvent)> mUpCallback;
    std::function<void(ci::app::MouseEvent)> mDownCallback;
    std::function<void(ci::app::MouseEvent)> mMoveCallback;
    std::function<void(ci::app::MouseEvent)> mDragCallback;
};
