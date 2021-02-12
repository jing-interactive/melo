#pragma once

#include "../3rdparty/cgltf/cgltf.h"
#include "../include/Node.h"
#include <filesystem>

namespace fs = std::filesystem;

typedef std::shared_ptr<struct GltfData> GltfDataRef;

struct GltfSubMesh
{
    typedef std::shared_ptr<GltfSubMesh> Ref;

    static Ref create(GltfDataRef data, const cgltf_primitive& property);

    cgltf_primitive property;
};

struct GltfMesh
{
    typedef std::shared_ptr<GltfMesh> Ref;

    static Ref create(GltfDataRef data, const cgltf_mesh& property);
    
    cgltf_mesh property;

    std::vector<GltfSubMesh::Ref> subMeshes;
};

struct GltfTexture
{
    typedef std::shared_ptr<GltfTexture> Ref;

    static Ref create(GltfDataRef data, const cgltf_texture& property);
    virtual ~GltfTexture();
    
    cgltf_texture property;

    int w = 0;
    int h = 0;
    int comp = 4;
    uint8_t* pixels = nullptr;
};

struct GltfAccessor
{
    typedef std::shared_ptr<GltfAccessor> Ref;
    static Ref create(GltfDataRef data, const cgltf_accessor& property)
    {
        auto ref = std::make_shared<GltfAccessor>();

        return ref;
    }
};

struct GltfBuffer
{
    typedef std::shared_ptr<GltfBuffer> Ref;
    static Ref create(GltfDataRef data, const cgltf_buffer& property);

    uint8_t* data = nullptr;
    int size;
};

struct GltfBufferView
{
    typedef std::shared_ptr<GltfBufferView> Ref;
    static Ref create(GltfDataRef data, const cgltf_buffer_view& property);

    cgltf_buffer_view property;
    GltfBuffer::Ref buffer;
};

struct GltfMaterial
{
    typedef std::shared_ptr<GltfMaterial> Ref;
    static Ref create(GltfDataRef data, const cgltf_material& property)
    {
        auto ref = std::make_shared<GltfMaterial>();

        return ref;
    }
};

struct GltfNode : melo::Node
{
    typedef std::shared_ptr<GltfNode> Ref;
    static Ref create(GltfDataRef data, const cgltf_node& property);

    void setup() override;

    GltfMesh::Ref mesh;
    cgltf_node property;
    GltfDataRef data;
};

struct GltfScene : GltfNode
{
    typedef std::shared_ptr<GltfScene> Ref;
    static Ref create(GltfDataRef data, const cgltf_scene& property);
};

struct GltfData : melo::Node
{
    static GltfDataRef create(const fs::path& meshPath);

    virtual ~GltfData()
    {
        if (property)
            cgltf_free(property);
    }

    cgltf_data* property = nullptr;
    fs::path path;

    std::vector<GltfMaterial::Ref> materials;
    std::vector<GltfAccessor::Ref> accessors;
    std::vector<GltfBuffer::Ref> buffers;
    std::vector<GltfBufferView::Ref> bufferViews;
    std::vector<GltfSubMesh::Ref> subMeshes;
    std::vector<GltfMesh::Ref> meshes;
    std::vector<GltfTexture::Ref> textures;

    std::vector<GltfNode::Ref> nodes;
};
