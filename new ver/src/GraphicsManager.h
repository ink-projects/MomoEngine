#pragma once

#include <webgpu/webgpu.h>
struct GLFWwindow;

#include <unordered_map>
#include <string>

namespace momoengine {

    class GraphicsManager {
    public:
        bool Startup(int window_width, int window_height, const char* window_name, bool fullscreen);
        void Shutdown();
        bool LoadTexture(const std::string& name, const std::string& path);
        void Draw(const std::vector<Sprite>& sprites);

        GLFWwindow* GetWindow() const { return window; }

    private:
        GLFWwindow* window = nullptr;

        WGPUInstance instance = nullptr;
        WGPUSurface surface = nullptr;
        WGPUAdapter adapter = nullptr;
        WGPUDevice device = nullptr;
        WGPUQueue queue = nullptr;

        WGPUBuffer vertex_buffer = nullptr;
        WGPUBuffer instance_buffer = nullptr;
        WGPUBuffer uniform_buffer = nullptr;
        
        WGPUShaderModule shader_module = nullptr;
        WGPURenderPipeline pipeline = nullptr;

        WGPUSampler sampler = nullptr;

        std::unordered_map<std::string, WGPUTextureView> textures;
    };

}
