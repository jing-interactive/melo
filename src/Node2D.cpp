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

#include "../include/Node2D.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

namespace nodes {

Node2D::Node2D( void )
    : mPosition( 0 )
    , mScale( 1 )
    , mAnchor( 0 )
    , mAnchorIsPercentage( false )
{
    setName("Node2D");
}

Node2D::~Node2D( void )
{
}

glm::vec2 Node2D::screenToParent( const glm::vec2 &pt ) const
{
	glm::vec2 p = pt;

	Node2DRef node = getParent<Node2D>();
	if( node )
		p = node->screenToObject( p );

	return p;
}

glm::vec2 Node2D::screenToObject( const glm::vec2 &pt, float z ) const
{
	// Build the viewport (x, y, width, height).
	glm::vec2 offset = gl::getViewport().first;
	glm::vec2 size = gl::getViewport().second;
	glm::vec4 viewport = glm::vec4( offset.x, offset.y, size.x, size.y );

	// Calculate the view-projection matrix.
	glm::mat4 model = getWorldTransform();
	glm::mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();

	// Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
	glm::vec3 near = glm::unProject( glm::vec3( pt.x, size.y - pt.y - 1, 0 ), model, viewProjection, viewport );
	glm::vec3 far = glm::unProject( glm::vec3( pt.x, size.y - pt.y - 1, 1 ), model, viewProjection, viewport );

	// Calculate world position.
	return glm::vec2( ci::lerp( near, far, ( z - near.z ) / ( far.z - near.z ) ) );
}

glm::vec2 Node2D::parentToScreen( const glm::vec2 &pt ) const
{
	glm::vec2 p = pt;

	Node2DRef node = getParent<Node2D>();
	if( node )
		p = node->objectToScreen( p );

	return p;
}

glm::vec2 Node2D::parentToObject( const glm::vec2 &pt ) const
{
	glm::mat4 invTransform = glm::inverse( getTransform() );
	glm::vec4 p = invTransform * glm::vec4( pt, 0, 1 );

	return glm::vec2( p.x, p.y );
}

glm::vec2 Node2D::objectToParent( const glm::vec2 &pt ) const
{
	glm::vec4 p = getTransform() * glm::vec4( pt, 0, 1 );
	return glm::vec2( p.x, p.y );
}

glm::vec2 Node2D::objectToScreen( const glm::vec2 &pt ) const
{
	// Build the viewport (x, y, width, height).
	glm::vec2 offset = gl::getViewport().first;
	glm::vec2 size = gl::getViewport().second;
	glm::vec4 viewport = glm::vec4( offset.x, offset.y, size.x, size.y );

	// Calculate the view-projection matrix.
	glm::mat4 model = getWorldTransform();
	glm::mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();

	glm::vec2 p = glm::vec2( glm::project( glm::vec3( pt, 0 ), model, viewProjection, viewport ) );
	p.y = size.y - 1 - p.y;

	return p;
}
} // namespace nodes