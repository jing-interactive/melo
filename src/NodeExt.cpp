#include "NodeExt.h"
#include "cinder/GeomIo.h"
#include "cinder/gl/gl.h"
#include "cinder/TriMesh.h"

using namespace std;
using namespace melo;

DirectionalLightNode::Ref DirectionalLightNode::create(float radius, Color color)
{
    auto ref = std::make_shared<DirectionalLightNode>();
    ref->setName("DirectionalLightNode");
    ref->radius = radius;
    ref->color = color;

    return ref;
}

void DirectionalLightNode::draw()
{
    static auto shader = gl::getStockShader(gl::ShaderDef().lambert());
    shader->bind();
    gl::color(color);
    gl::drawSphere({}, radius);
}

GridNode::Ref GridNode::create(float meters)
{
    return std::make_shared<GridNode>(meters);
}

GridNode::GridNode(float meters) : mMeters(meters)
{
    shader = gl::getStockShader(gl::ShaderDef().color());

#if 0
    vertBatch = gl::Batch::create(geom::WirePlane().size(vec2(meters)).subdivisions(ivec2(meters/3)), shader);
#else
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
#endif

    setName("GridNode");
}

void GridNode::draw()
{
    gl::ScopedDepthTest depthTest(false);
    gl::ScopedGlslProg glsl(shader);
    vertBatch->draw();
    gl::drawCoordinateFrame(mMeters * 0.5f, mMeters * 0.05f, mMeters * 0.005f);
}

BuiltinMeshNode::Ref BuiltinMeshNode::create(TriMeshRef triMesh)
{
    return std::make_shared<BuiltinMeshNode>(triMesh);
}

BuiltinMeshNode::BuiltinMeshNode(TriMeshRef triMesh)
{
    vboMesh = gl::VboMesh::create(*triMesh);
    shader = gl::getStockShader(gl::ShaderDef().color());
    setName("BuiltinMeshNode");
}

BuiltinMeshNode::Ref BuiltinMeshNode::create(const geom::Source& source)
{
    auto triMesh = TriMesh::create(source);
    return make_shared<BuiltinMeshNode>(triMesh);
}

void BuiltinMeshNode::draw()
{
    if (vboMesh)
    {
        gl::ScopedGlslProg glsl(shader);
        gl::draw(vboMesh);
    }
}