#pragma once

#include <string>
#include <unordered_map>
#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <vector>
#include "Sprite.h"

struct GLFWwindow;

WGPUTextureFormat wgpuSurfaceGetPreferredFormat(WGPUSurface surface, WGPUAdapter adapter);

namespace momoengine {

    struct InstanceData {
        glm::vec3 translation;
        glm::vec2 scale;
        //rotation?
    };

    //for loading textures!
    struct Texture {
        WGPUTexture texture;
        WGPUTextureView view;
        WGPUSampler sampler;
        int width;
        int height;
    };

    struct Uniforms {
        glm::mat4 projection;
    };

    class GraphicsManager {

    public:
        
        bool Startup(int window_width, int window_height, const char* window_name, bool fullscreen);
        void Shutdown();
        bool LoadTexture(const std::string& name, const std::string& path);
        void Draw(const std::vector<class Sprite>& sprites);

        GLFWwindow* GetWindow() const { return window; }

    private:
        GLFWwindow* window = nullptr;
        std::unordered_map<std::string, Texture> textures;

        //WebGPU objects

        WGPUInstance instance = nullptr;
        WGPUSurface surface = nullptr;
        WGPUAdapter adapter = nullptr;
        WGPUDevice device = nullptr;
        WGPUQueue queue = nullptr;
        WGPUBuffer vertexBuffer = nullptr;
        WGPUBuffer instanceBuffer = nullptr;
        WGPUShaderModule shader_module = nullptr;
        WGPURenderPipeline pipeline = nullptr;
        WGPUBuffer uniformBuffer = nullptr;

    };

}
