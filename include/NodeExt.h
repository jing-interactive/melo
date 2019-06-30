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

#include "Node3D.h"

namespace nodes
{
    using namespace ci;

    struct GridNode : public Node3D
    {
        gl::VertBatchRef vertBatch;
        float mMeters;

        GridNode(float meters = 10.0f) : mMeters(meters)
        {
            vertBatch = gl::VertBatch::create(GL_LINES);
            vertBatch->begin(GL_LINES);
            for (int i = -meters; i <= meters; ++i)
            {
                vertBatch->color(Color(0.25f, 0.25f, 0.25f));
                vertBatch->color(Color(0.25f, 0.25f, 0.25f));
                vertBatch->color(Color(0.25f, 0.25f, 0.25f));
                vertBatch->color(Color(0.25f, 0.25f, 0.25f));

                vertBatch->vertex(float(i), 0.0f, -meters);
                vertBatch->vertex(float(i), 0.0f, +meters);
                vertBatch->vertex(-meters, 0.0f, float(i));
                vertBatch->vertex(+meters, 0.0f, float(i));
            }
            vertBatch->end();

            setName("GridNode");
        }

        void draw()
        {
            gl::ScopedDepthWrite depthWrite(false);
            gl::ScopedGlslProg glsl(am::glslProg("color"));
            vertBatch->draw();
            gl::drawCoordinateFrame(mMeters * 0.1f, mMeters * 0.01f, mMeters * 0.001f);
        }

        virtual inline std::string toString() const { return "GridNode"; }
    };

    struct TriMeshNode : public Node3D
    {
        TriMeshRef triMesh;

        TriMeshNode(TriMeshRef triMesh) : triMesh(triMesh) {}

        void draw()
        {
            if (triMesh)
            {
                gl::draw(*triMesh);
            }
        }

        virtual inline std::string toString() const { return "TriMeshNode"; }
    };
} // namespace nodes