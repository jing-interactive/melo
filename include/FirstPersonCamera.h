#pragma once

#include "cinder/app/App.h"

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera/flythrough_camera.h"

using namespace ci;
using namespace ci::app;

struct FirstPersonCamera : public CameraPersp
{
    double mElapsedSeconds;
    ivec2 mMousePos, mPrevMousePos;

    bool mIsKeyPressed[KeyEvent::KEY_LAST] = { false };
    bool mIsRMousePressed = false;

    vec3 eye = { 3.0f, 3.0f, 3.0f };
    vec3 look = { 0.0f, 0.0f, 1.0f };
    const vec3 up = { 0.0f, 1.0f, 0.0f };
    float eye_speed = 3.0f;
    float degrees_per_cursor_move = 0.2f;
    float max_pitch_rotation_degrees = 80.0f;

    bool activated = true;

    void setActive(bool flag)
    {
        activated = flag;
    }

    void setup()
    {
        mElapsedSeconds = getElapsedSeconds();
        //        mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());

        //        flythrough_camera_look_to(pos, look, up, glm::value_ptr(mViewMatrix), 0);

        getWindow()->getSignalResize().connect([&] { setAspectRatio(getWindowAspectRatio()); });

        getWindow()->getSignalKeyDown().connect(
            [&](KeyEvent& event) { mIsKeyPressed[event.getCode()] = true; });

        getWindow()->getSignalKeyUp().connect(
            [&](KeyEvent& event) { mIsKeyPressed[event.getCode()] = false; });

        getWindow()->getSignalMouseDown().connect(
            [&](MouseEvent& ev) { mIsRMousePressed = ev.isRight(); });

        getWindow()->getSignalMouseUp().connect([&](MouseEvent& ev) { mIsRMousePressed = false; });

        getWindow()->getSignalMouseMove().connect([&](MouseEvent& ev) {
            mIsRMousePressed = ev.isRight();
            mMousePos = ev.getPos();
        });

        AppBase::get()->getSignalUpdate().connect([&] {
            float delta_time_sec = getElapsedSeconds() - mElapsedSeconds;
            mElapsedSeconds = getElapsedSeconds();

            flythrough_camera_update(
                glm::value_ptr(eye), glm::value_ptr(look), glm::value_ptr(up),
                glm::value_ptr(mViewMatrix), delta_time_sec,
                eye_speed * (mIsKeyPressed[KeyEvent::KEY_LSHIFT] ? 2.0f : 1.0f) * activated,
                degrees_per_cursor_move, max_pitch_rotation_degrees, mMousePos.x - mPrevMousePos.x,
                mMousePos.y - mPrevMousePos.y, 
                mIsKeyPressed[KeyEvent::KEY_w], mIsKeyPressed[KeyEvent::KEY_a], mIsKeyPressed[KeyEvent::KEY_s], mIsKeyPressed[KeyEvent::KEY_d], 
                false /*mIsKeyPressed[KeyEvent::KEY_SPACE]*/,
                mIsKeyPressed[KeyEvent::KEY_LCTRL], 0);

            if (activated)
            {
                float* view = glm::value_ptr(mViewMatrix);
                printf("\n");
                printf("pos: %f, %f, %f\n", eye[0], eye[1], eye[2]);
                printf("look: %f, %f, %f\n", look[0], look[1], look[2]);
                printf("view: %f %f %f %f\n"
                    "      %f %f %f %f\n"
                    "      %f %f %f %f\n"
                    "      %f %f %f %f\n",
                    view[0], view[1], view[2], view[3], view[4], view[5], view[6], view[7],
                    view[8], view[9], view[10], view[11], view[12], view[13], view[14],
                    view[15]);
                mModelViewCached = true;
            }
            mPrevMousePos = mMousePos;
        });
    }
};
