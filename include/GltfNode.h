#pragma once

#include "../3rdparty/cgltf/cgltf.h"
#include "../include/Node.h"
#include <filesystem>

namespace fs = std::filesystem;

typedef std::shared_ptr<struct GltfData> GltfDataRef;

struct GltfMesh
{
    typedef std::shared_ptr<GltfMesh> Ref;

    static Ref create(GltfDataRef dataRef, const cgltf_mesh& property)
    {
        auto ref = std::make_shared<GltfMesh>();

        return ref;
    }
};

struct GltfTexture
{
    typedef std::shared_ptr<GltfTexture> Ref;

    static Ref create(GltfDataRef dataRef, const cgltf_texture& property);
    
    cgltf_texture property;

    int w = 0;
    int h = 0;
    int comp = 4;
    uint8_t* pixels = nullptr;
};

struct GltfAccessor
{
    typedef std::shared_ptr<GltfAccessor> Ref;
    static Ref create(GltfDataRef dataRef, const cgltf_accessor& property)
    {
        auto ref = std::make_shared<GltfAccessor>();

        return ref;
    }
};

struct GltfBuffer
{
    typedef std::shared_ptr<GltfBuffer> Ref;
    static Ref create(GltfDataRef dataRef, const cgltf_buffer& property)
    {
        auto ref = std::make_shared<GltfBuffer>();

        return ref;
    }
};

struct GltfBufferView
{
    typedef std::shared_ptr<GltfBufferView> Ref;
    static Ref create(GltfDataRef dataRef, const cgltf_buffer_view& property)
    {
        auto ref = std::make_shared<GltfBufferView>();

        return ref;
    }
};

struct GltfMaterial
{
    typedef std::shared_ptr<GltfMaterial> Ref;
    static Ref create(GltfDataRef dataRef, const cgltf_material& property)
    {
        auto ref = std::make_shared<GltfMaterial>();

        return ref;
    }
};

struct GltfNode : melo::Node
{
    typedef std::shared_ptr<GltfNode> Ref;
    static Ref create(GltfDataRef dataRef, const cgltf_node& property)
    {
        auto ref = std::make_shared<GltfNode>();

        return ref;
    }
};

struct GltfData : melo::Node
{
    static GltfDataRef create(const fs::path& meshPath);

    virtual ~GltfData()
    {
        if (data)
            cgltf_free(data);
    }

    cgltf_data* data = nullptr;
    fs::path meshPath;

    std::vector<GltfMaterial::Ref> materials;
    std::vector<GltfAccessor::Ref> accessors;
    std::vector<GltfBuffer::Ref> buffers;
    std::vector<GltfBufferView::Ref> bufferViews;
    std::vector<GltfMesh::Ref> meshes;
    std::vector<GltfTexture::Ref> textures;
};
