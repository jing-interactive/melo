#include "../include/cigltf.h"
#include "../include/melo.h"

using namespace std;

namespace melo
{
    NodeRef loadScene(const std::string& filename)
    {
        NodeRef root;
        auto tree = ModelGLTF::create(filename);
        if (tree)
        {
            root = tree->currentScene->getChildren()[0];
            auto& children = root->getChildren();
            for (int i = 0; i < children.size(); i++)
            {
                children[i] = melo::create(children[i]->getName());
            }
        }

        return root;
    }

    static void addNode(NodeRef node, int& nodeidx, tinygltf::Model& gltfmodel, tinygltf::Scene& gltfscene) {
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
            for (auto& child : node->getChildren())
            {
                addNode(child, nodeidx, gltfmodel, gltfscene);
                gltfmodel.nodes[idx].children.push_back(nodeidx);
            }
        }
        nodeidx++;
    };

    bool writeScene(NodeRef scene, const std::string& filename)
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
}