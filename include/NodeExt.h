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

#include "Node.h"
#include "cinder/gl/gl.h"

namespace cinder {
    namespace geom {
        class Source;
    }
    typedef std::shared_ptr<class TriMesh>		TriMeshRef;
}

namespace melo
{
    using namespace ci;

    struct DirectionalLightNode : public Node
    {
        typedef std::shared_ptr<DirectionalLightNode> Ref;
        static Ref create(float radius = 5, Color color = { 1,1,1 });

        void draw(DrawOrder order) override;

        float radius;
        Color color;
    };

    struct GridNode : public Node
    {
        typedef std::shared_ptr<GridNode> Ref;

        gl::VertBatchRef vertBatch;
        gl::GlslProgRef shader;
        float mMeters;

        static Ref create(float meters = 10.0f);

        GridNode(float meters = 10.0f);

        void draw(DrawOrder order) override;
    };

    struct MeshNode : public Node
    {
        typedef std::shared_ptr<MeshNode> Ref;

        gl::VboMeshRef vboMesh;
        gl::GlslProgRef shader;

        static Ref create(TriMeshRef triMesh);

        static Ref create(const geom::Source& source);
        MeshNode(TriMeshRef triMesh);

        void draw(DrawOrder order) override;
    };
} // namespace nodes