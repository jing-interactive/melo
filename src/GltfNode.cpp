#include "../include/GltfNode.h"

#include "../3rdparty/tinygltf/stb_image.h"

GltfSubMesh::Ref GltfSubMesh::create(GltfDataRef data, const cgltf_primitive& property)
{
    auto ref = std::make_shared<GltfSubMesh>();
    ref->property = property;

    return ref;
}

GltfMesh::Ref GltfMesh::create(GltfDataRef data, const cgltf_mesh& property)
{
    auto ref = std::make_shared<GltfMesh>();
    ref->property = property;
    for (auto i = 0; i < property.primitives_count; i++)
    {
        ref->subMeshes.emplace_back(GltfSubMesh::create(data, property.primitives[i]));
    }
    for (auto i = 0; i < property.weights_count; i++)
    {
        // TODO:skin
    }

    return ref;
}

GltfTexture::~GltfTexture()
{
    if (pixels)
    {
        stbi_image_free(pixels);
    }
}

GltfTexture::Ref GltfTexture::create(GltfDataRef data, const cgltf_texture& property)
{
    auto ref = std::make_shared<GltfTexture>();
    ref->property = property;

    if (property.image->uri)
    {
        auto newPath = data->path.parent_path() / property.image->uri;
        ref->pixels = stbi_load(newPath.string().c_str(), &ref->w, &ref->h, &ref->comp, 0);
    }
    else
    {
        // in memory
    }

    return ref;
}

GltfBufferView::Ref GltfBufferView::create(GltfDataRef data, const cgltf_buffer_view& property)
{
    auto ref = std::make_shared<GltfBufferView>();
    ref->buffer = data->buffers[property.buffer - data->property->buffers];
    ref->property = property;

    return ref;
}

GltfBuffer::Ref GltfBuffer::create(GltfDataRef data, const cgltf_buffer& property)
{
    auto ref = std::make_shared<GltfBuffer>();
    if (property.uri)
    {
        ref->data = (uint8_t*)property.data;
        ref->size = property.size;
    }

    return ref;
}

GltfNode::Ref GltfNode::create(GltfDataRef data, const cgltf_node& property)
{
    auto ref = std::make_shared<GltfNode>();
    ref->property = property;
    ref->data = data;

    if (property.mesh)
    {
        ref->mesh = data->meshes[property.mesh - data->property->meshes];
        ref->setName(property.mesh->name);
    }

    if (property.has_matrix)
    {
        ref->setConstantTransform(glm::make_mat4x4(property.matrix));
    }
    if (property.has_translation)
    {
        ref->setPosition(
            { property.translation[0], property.translation[1], property.translation[2] });
    }
    if (property.has_scale)
    {
        ref->setScale({ property.scale[0], property.scale[1], property.scale[2] });
    }
    if (property.has_rotation)
    {
        ref->setRotation(glm::make_quat(property.rotation));
    }

    if (property.name)
        ref->setName(property.name);

    return ref;
}

void GltfNode::setup()
{
    for (auto i = 0; i < property.children_count; i++)
    {
        addChild(data->nodes[property.children[i] - data->property->nodes]);
    }
}

GltfScene::Ref GltfScene::create(GltfDataRef data, const cgltf_scene& property)
{
    auto ref = std::make_shared<GltfScene>();
    if (property.name)
        ref->setName(property.name);
    for (auto i = 0; i < property.nodes_count; i++)
    {
        ref->addChild(data->nodes[property.nodes[i] - data->property->nodes]);
    }

    return ref;
}

GltfDataRef GltfData::create(const fs::path& path)
{
    auto ref = std::make_shared<GltfData>();
    cgltf_result ret = cgltf_result_success;
    cgltf_options options = {};
    ret = cgltf_parse_file(&options, path.string().c_str(), &ref->property);

    if (ret != cgltf_result_success)
        return {};
    
    ret = cgltf_load_buffers(&options, ref->property, path.string().c_str());
    if (ret != cgltf_result_success)
        return {};

    ref->path = path;
    auto data = ref->property; // make shortcut
    for (auto i = 0; i < data->buffers_count; i++)
        ref->buffers.emplace_back(GltfBuffer::create(ref, data->buffers[i]));
    for (auto i = 0; i < data->buffer_views_count; i++)
        ref->bufferViews.emplace_back(GltfBufferView::create(ref, data->buffer_views[i]));
    for (auto i = 0; i < data->accessors_count; i++)
        ref->accessors.emplace_back(GltfAccessor::create(ref, data->accessors[i]));
    for (auto i = 0; i < data->meshes_count; i++)
        ref->meshes.emplace_back(GltfMesh::create(ref, data->meshes[i]));

    for (auto i = 0; i < data->textures_count; i++)
        ref->textures.emplace_back(GltfTexture::create(ref, data->textures[i]));
    for (auto i = 0; i < data->materials_count; i++)
        ref->materials.emplace_back(GltfMaterial::create(ref, data->materials[i]));

    char name[100];
    for (auto i = 0; i < data->nodes_count; i++)
    {
        auto& node = GltfNode::create(ref, data->nodes[i]);
        if (data->nodes[i].name == nullptr)
        {
            sprintf(name, "node_%d", i);
            node->setName(name);
        }
        ref->nodes.emplace_back(node);
    }

    return ref;
}
