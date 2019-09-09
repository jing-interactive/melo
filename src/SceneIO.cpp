#include "SceneIO.h"
#include "../include/cigltf.h"

using namespace std;
using namespace nodes;

Node3DRef nodes::loadSceneFromGLTF(const std::string& filename)
{
    auto ref = ModelGLTF::create(filename);
    if (ref)
        return ref->currentScene->getChildren<nodes::Node3D>()[0];

    return {};
}

void addNode(Node3DRef node, int& nodeidx, tinygltf::Model& gltfmodel, tinygltf::Scene& gltfscene) {
    tinygltf::Node gltfnode;
    gltfnode.name = node->getName();
    auto ptr = glm::value_ptr(node->getTransform());
    gltfnode.matrix = { ptr, ptr + 16 };

    tinygltf::Value::Object attributes;
    attributes["visible"] = tinygltf::Value(node->isVisible());
    gltfnode.extras = tinygltf::Value(attributes);
    gltfmodel.nodes.emplace_back(gltfnode);
    int idx = nodeidx;

    auto key = node->getName().find(".gltf");
    if (key == string::npos)
    {
        for (auto& child : node->getChildren<Node3D>())
        {
            addNode(child, nodeidx, gltfmodel, gltfscene);
            gltfmodel.nodes[idx].children.push_back(nodeidx);
        }
    }
    nodeidx++;
};

bool nodes::writeSceneToGLTF(Node3DRef scene, const std::string& filename)
{
    tinygltf::Model gltfmodel;
    gltfmodel.asset.generator = "melo runtime";
    gltfmodel.asset.version = "1.0";
    gltfmodel.defaultScene = 0;
    tinygltf::Scene gltfscene;
    gltfscene.name = "A melo scene";

    int nodeidx = 0;
    gltfscene.nodes.push_back(nodeidx); // "scene" only contains root node (aka #0)

    addNode(scene, nodeidx, gltfmodel, gltfscene);

    gltfmodel.scenes.emplace_back(gltfscene);

    //std::string name;
    //std::vector<int> nodes;

    //ExtensionMap extensions;
    //Value extras;
    tinygltf::TinyGLTF factory;
    auto result = factory.WriteGltfSceneToFile(&gltfmodel, filename,
        false, false, true, false);

    return result;
}
