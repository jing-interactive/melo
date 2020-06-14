// https://quaternions.online/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/CameraUi.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "MiniConfigImgui.h"

#include "cigltf.h"
#include "melo.h"

#include <glm/gtc/quaternion.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

struct AnimToCSVApp : public App
{
    ModelGLTFRef        gltfNode;
    AnimationGLTF::Ref  mPickedAnimation;
    melo::NodeRef       mScene;
    melo::NodeRef       mGridNode;

    CameraPersp         mCam;
    CameraUi            mCamUi;

    void setup() override
    {
        log::makeLogger<log::LoggerFileRotating>(fs::path(), "IG.%Y.%m.%d.log");

        auto& args = getCommandLineArgs();

        if (args.size() <= 1)
        {
            CI_LOG_E("Usage: AnimToCSV.exe /path/to/model.gltf");
            quit();
            return;
        }

        // /path/to/MeloViewer.exe file.obj
        auto filePath = args[1];
        std::string loadingError;
        bool loadAnimationOnly = true;
        gltfNode = ModelGLTF::create(filePath, &loadingError, loadAnimationOnly);
        if (!gltfNode)
        {
            CI_LOG_E("Failed to open this gltf file. Reason: " << loadingError);
            quit();
            return;
        }

        {
            auto csvFile = filePath + ".csv";
            FILE* fp = fopen(csvFile.c_str(), "w");
            if (!fp)
            {
                CI_LOG_E("Failed to open " << csvFile);
                quit();
                return;
            }

            fprintf(fp, "x,y,z,pitch,yaw,roll,qx,qy,qz,qw\n");

            for (const auto& anim : gltfNode->animations)
            {
                const auto& samplers = anim->samplers;
                const auto& channels = anim->channels;
                const AnimationSampler* T = nullptr;
                const AnimationSampler* R = nullptr;
                const AnimationSampler* S = nullptr;
                for (auto& channel : channels)
                {
                    auto& sampler = samplers[channel.samplerIndex];
                    if (channel.path == AnimationChannel::TRANSLATION)
                        T = &sampler;
                    else if (channel.path == AnimationChannel::ROTATION)
                        R = &sampler;
                    else if (channel.path == AnimationChannel::SCALE)
                        S = &sampler;
                }

                CI_ASSERT(T && T->inputs.size() == T->outputsVec4.size());
                int size = T->inputs.size();
                for (int i = 0; i < size; i++)
                {
                    glm::quat q = glm::make_quat(&R->outputsVec4[i].x);
                    auto euler = glm::degrees(glm::eulerAngles(q));
                    fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
                        T->outputsVec4[i].x, T->outputsVec4[i].y, T->outputsVec4[i].z,
                        euler.x, euler.y, euler.z,
                        R->outputsVec4[i].x, R->outputsVec4[i].y, R->outputsVec4[i].z, R->outputsVec4[i].w);
                }

                fclose(fp);

                // only dumps the first anim
                break;
            }
        }

        createConfigImgui();
        gl::enableDepth();

        mScene = melo::createRootNode();

        mGridNode = melo::createGridNode(100.0f);
        mScene->addChild(mGridNode);

        mCam.setFarClip(10000);
        mCamUi = CameraUi(&mCam, getWindow(), -1);
    
        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        getSignalCleanup().connect([&] { writeConfig(); });

        getSignalUpdate().connect([&] {
            mScene->treeUpdate();
        });
        
        getWindow()->getSignalDraw().connect([&] {
            gl::clear();
            gl::setMatrices(mCam);

            mScene->treeDraw();

            vec3 T;
            quat R;
            vec3 S;

            if (ImGui::Begin("Anim"))
            {
                for (auto& anim : gltfNode->animations)
                {
                    if (ImGui::Button(anim->name.c_str()))
                    {
                        mPickedAnimation = anim;
                        anim->apply();
                    }
                }

                if (mPickedAnimation)
                {
                    for (auto& channel : mPickedAnimation->channels)
                    {
                        if (channel.path == AnimationChannel::TRANSLATION)
                        {
                            ImGui::Text("Duraton: %f s", channel.translation.getParent()->getDuration());
                            T = channel.translation.value();
                            ImGui::DragFloat3(channel.property.target_path.c_str(), &T);
                        }
                        else if (channel.path == AnimationChannel::ROTATION)
                        {
                            R = channel.rotation.value();
                            ImGui::DragFloat4(channel.property.target_path.c_str(), (vec4*)&R);
                        }
                        else if (channel.path == AnimationChannel::SCALE)
                        {
                            S = channel.scale.value();
                            ImGui::DragFloat3(channel.property.target_path.c_str(), &S);
                        }
                    }
                }
                ImGui::End();
            }

            {
                glm::mat4 transform = glm::translate(T);
                transform *= glm::toMat4(R);
                transform *= glm::scale(vec3{3.0f,3.0f,3.0f });
                gl::ScopedModelMatrix mdl(transform);
                gl::ScopedGlslProg prog(am::glslProg("lambert"));
                gl::draw(am::vboMesh("Teapot"));
                gl::drawCoordinateFrame(2);
            }

        });
    }
};

CINDER_APP( AnimToCSVApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
} )
