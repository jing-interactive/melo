#include "../include/GltfNode.h"

#include "../3rdparty/tinygltf/stb_image.h"

GltfImage::Ref GltfImage::create(GltfDataRef dataRef, const cgltf_image& property)
{
    auto ref = std::make_shared<GltfImage>();
    if (property.uri != nullptr)
    {
        auto folder = dataRef->meshPath.parent_path();

    }
    else
    {
        // in memory
    }
    return ref;
}

GltfDataRef GltfData::create(const fs::path& meshPath)
{
    auto ref = std::make_shared<GltfData>();
    cgltf_result ret = cgltf_result_success;
    cgltf_options options = {};
    ret = cgltf_parse_file(&options, meshPath.string().c_str(), &ref->data);

    if (ret != cgltf_result_success)
        return {};

    ref->meshPath = meshPath;
    auto data = ref->data; // make shortcut
    for (auto i = 0; i < data->accessors_count; i++)
        ref->accessors.emplace_back(GltfAccessor::create(ref, data->accessors[i]));
    for (auto i = 0; i < data->buffers_count; i++)
        ref->buffers.emplace_back(GltfBuffer::create(ref, data->buffers[i]));
    for (auto i = 0; i < data->buffer_views_count; i++)
        ref->bufferViews.emplace_back(GltfBufferView::create(ref, data->buffer_views[i]));
    for (auto i = 0; i < data->meshes_count; i++)
        ref->meshes.emplace_back(GltfMesh::create(ref, data->meshes[i]));

    for (auto i = 0; i < data->images_count; i++)
        ref->images.emplace_back(GltfImage::create(ref, data->images[i]));
    for (auto i = 0; i < data->samplers_count; i++)
        ref->samplers.emplace_back(GltfSampler::create(ref, data->samplers[i]));
    for (auto i = 0; i < data->textures_count; i++)
        ref->textures.emplace_back(GltfTexture::create(ref, data->textures[i]));
    for (auto i = 0; i < data->materials_count; i++)
        ref->materials.emplace_back(GltfMaterial::create(ref, data->materials[i]));

    return ref;
}
