#define GLFW_INCLUDE_NONE  //doesn't include OpenGL
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GraphicsManager.h"
#include "spdlog/spdlog.h"
#include "Sprite.h"

#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm> //for std::sort
#include <utility> //for std::pair


using namespace glm;

//replacement for wgpuSurfaceGetPreferredFormat() defunct function
WGPUTextureFormat wgpuSurfaceGetPreferredFormat(WGPUSurface surface, WGPUAdapter adapter) {
    WGPUSurfaceCapabilities capabilities{};
    wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
    const WGPUTextureFormat result = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return result;
}

namespace momoengine {

    template< typename T > constexpr const T* to_ptr(const T& val) { return &val; }
    template< typename T, std::size_t N > constexpr const T* to_ptr(const T(&& arr)[N]) { return arr; }

    // A vertex buffer containing a textured square.
    const struct {
        // position
        float x, y;
        // texcoords
        float u, v;
    } vertices[] = {
        // position       // texcoords
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f, 0.0f },
    };

    bool GraphicsManager::Startup(int window_width, int window_height, const char* window_name, bool fullscreen) {
        if (!glfwInit()) {
            spdlog::error("Failed to initialize GLFW.");
            return false;
        }

        // We don't want GLFW to set up a graphics API.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Create the window.
        window = glfwCreateWindow(window_width, window_height, window_name, fullscreen ? glfwGetPrimaryMonitor() : 0, 0);
        if (!window) {  //error catching for if something happened with creating the window
            spdlog::error("Failed to create a window.");
            glfwTerminate();
            return false;
        }

        //creates a window
        glfwSetWindowAspectRatio(window, window_width, window_height);
        spdlog::info("Window created: {}x{}", window_width, window_height);

        //instance of a struct pointer for WebGPU
        instance = wgpuCreateInstance(to_ptr(WGPUInstanceDescriptor{}));
        if (!instance) {
            spdlog::error("Failed to create WebGPU instance.");
            glfwTerminate();
            return false;
        }

        //WebGPU surface for the window
        surface = glfwCreateWindowWGPUSurface(instance, window);

        //WebGPU adapter; the physical GPU on the system
        adapter = nullptr;
        wgpuInstanceRequestAdapter(
            instance,
            to_ptr(WGPURequestAdapterOptions{ .featureLevel = WGPUFeatureLevel_Core, .compatibleSurface = surface }),
            WGPURequestAdapterCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapterParam, WGPUStringView message, void* adapter_ptr, void*) {
                    if (status != WGPURequestAdapterStatus_Success) {
                        spdlog::error("Failed to get a WebGPU adapter: {}", std::string_view(message.data, message.length));
                        return;
                    }
                    *static_cast<WGPUAdapter*>(adapter_ptr) = adapterParam;
                },
                .userdata1 = &adapter
            }
        );

        while (!adapter) {
            wgpuInstanceProcessEvents(instance);
        }
        spdlog::info("WebGPU adapter obtained.");

        //WebGPU device; the driver of the adapter
        device = nullptr;
        wgpuAdapterRequestDevice(
            adapter,
            to_ptr(WGPUDeviceDescriptor{
                .uncapturedErrorCallbackInfo = {.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
                    spdlog::error("WebGPU uncaptured error {}: {}", int(type), std::string_view(message.data, message.length));
                }}
                }),
            WGPURequestDeviceCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestDeviceStatus status, WGPUDevice deviceParam, WGPUStringView message, void* device_ptr, void*) {
                    if (status != WGPURequestDeviceStatus_Success) {
                        spdlog::error("Failed to get a WebGPU device: {}", std::string_view(message.data, message.length));
                        return;
                    }
                    *static_cast<WGPUDevice*>(device_ptr) = deviceParam;
                },
                .userdata1 = &device
            }
        );

        while (!device) {
            wgpuInstanceProcessEvents(instance);
        }
        spdlog::info("WebGPU device obtained.");

        //WebGPU queue associated with device
        queue = wgpuDeviceGetQueue(device);

        //configure the surface based on window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        WGPUSurfaceConfiguration config = {
            .device = device,
            .format = wgpuSurfaceGetPreferredFormat(surface, adapter),
            .usage = WGPUTextureUsage_RenderAttachment,
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .presentMode = WGPUPresentMode_Fifo // Explicitly set this because of a Dawn bug
        };

        wgpuSurfaceConfigure(surface, &config);
        spdlog::info("WebGPU surface configured: {}x{}", width, height);

        //create a vertex buffer
        vertexBuffer = wgpuDeviceCreateBuffer(
            device,
            to_ptr(WGPUBufferDescriptor{
                .label = WGPUStringView("Vertex Buffer", WGPU_STRLEN),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
                .size = sizeof(vertices)
                })
        );

        //upload the vertex data to the GPU
        wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertices, sizeof(vertices));
        spdlog::info("Vertex buffer created and uploaded ({} bytes).", sizeof(vertices));

        //create an instance buffer
        instanceBuffer = wgpuDeviceCreateBuffer(
            device,
            to_ptr(WGPUBufferDescriptor{
                .label = WGPUStringView("Instance Buffer", WGPU_STRLEN),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
                .size = sizeof(InstanceData)
                })
        );

        //create the uniform buffer
        uniformBuffer = wgpuDeviceCreateBuffer(
            device,
            to_ptr(WGPUBufferDescriptor{
                .label = WGPUStringView("Uniform Buffer", WGPU_STRLEN),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
                .size = sizeof(Uniforms)
                })
        );

        InstanceData instanceData{
            .translation = vec3(0.0f, 0.0f, 0.0f),
            .scale = vec2(1.0f, 1.0f)
        };

        wgpuQueueWriteBuffer(queue, instanceBuffer, 0, &instanceData, sizeof(InstanceData));
        spdlog::info("Instance buffer created and uploaded ({} bytes).", sizeof(InstanceData));

        //WGSL shaders
        const char* source = R"(
            struct Uniforms {
                projection: mat4x4f,
            };

            @group(0) @binding(0) var<uniform> uniforms: Uniforms;
            @group(0) @binding(1) var texSampler: sampler;
            @group(0) @binding(2) var texData: texture_2d<f32>;

            struct VertexInput {
                @location(0) position: vec2f,
                @location(1) texcoords: vec2f,
                @location(2) translation: vec3f,
                @location(3) scale: vec2f,
            };

            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) texcoords: vec2f,
            };

            @vertex
            fn vertex_shader_main( in: VertexInput ) -> VertexOutput {
                var out: VertexOutput;
                out.position = uniforms.projection * vec4f( vec3f( in.scale * in.position, 0.0 ) + in.translation, 1.0 );
                out.texcoords = in.texcoords;
                return out;
            }

            @fragment
            fn fragment_shader_main( in: VertexOutput ) -> @location(0) vec4f {
                let color = textureSample( texData, texSampler, in.texcoords ).rgba;
                return color;
            }
        )";

        //wrap WGSL in a descriptor
        WGPUShaderSourceWGSL source_desc = {};
        source_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        source_desc.code = WGPUStringView(source, strlen(source));

        //wrap in shader descriptor
        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = &source_desc.chain;

        shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);
        if (!shader_module) {
            spdlog::error("Failed to create shader module.");
            return false;
        }
        spdlog::info("Shader module created successfully.");

        pipeline = wgpuDeviceCreateRenderPipeline(device, to_ptr(WGPURenderPipelineDescriptor{

            // Describe the vertex shader inputs
            .vertex = {
                .module = shader_module,
                .entryPoint = WGPUStringView{ "vertex_shader_main", std::string_view("vertex_shader_main").length() },
                // Vertex attributes.
                .bufferCount = 2,
                .buffers = to_ptr<WGPUVertexBufferLayout>({
                // We have one buffer with our per-vertex position and UV data. This data never changes.
                // Note how the type, byte offset, and stride (bytes between elements) exactly matches our `vertex_buffer`.
                {
                    .stepMode = WGPUVertexStepMode_Vertex,
                    .arrayStride = 4 * sizeof(float),
                    .attributeCount = 2,
                    .attributes = to_ptr<WGPUVertexAttribute>({
                        // Position x,y are first.
                        {
                            .format = WGPUVertexFormat_Float32x2,
                            .offset = 0,
                            .shaderLocation = 0
                        },
                        // Texture coordinates u,v are second.
                        {
                            .format = WGPUVertexFormat_Float32x2,
                            .offset = 2 * sizeof(float),
                            .shaderLocation = 1
                        }
                        })
                },
                    // We will use a second buffer with our per-sprite translation and scale. This data will be set in our draw function.
                    {
                        // This data is per-instance. All four vertices will get the same value. Each instance of drawing the vertices will get a different value.
                        // The type, byte offset, and stride (bytes between elements) exactly match the array of `InstanceData` structs we will upload in our draw function.
                        .stepMode = WGPUVertexStepMode_Instance,
                        .arrayStride = sizeof(InstanceData),
                        .attributeCount = 2,
                        .attributes = to_ptr<WGPUVertexAttribute>({
                        // Translation as a 3D vector.
                        {
                            .format = WGPUVertexFormat_Float32x3,
                            .offset = offsetof(InstanceData, translation),
                            .shaderLocation = 2
                        },
                            // Scale as a 2D vector for non-uniform scaling.
                            {
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = offsetof(InstanceData, scale),
                                .shaderLocation = 3
                            }
                            })
                    }
                    })
                },

            // Interpret our 4 vertices as a triangle strip
            .primitive = WGPUPrimitiveState{
                .topology = WGPUPrimitiveTopology_TriangleStrip,
                },

                // No multi-sampling (1 sample per pixel, all bits on).
                .multisample = WGPUMultisampleState{
                    .count = 1,
                    .mask = ~0u
                    },

            // Describe the fragment shader and its output
            .fragment = to_ptr(WGPUFragmentState{
                .module = shader_module,
                .entryPoint = WGPUStringView{ "fragment_shader_main", std::string_view("fragment_shader_main").length() },

                // Our fragment shader outputs a single color value per pixel.
                .targetCount = 1,
                .targets = to_ptr<WGPUColorTargetState>({
                    {
                        .format = wgpuSurfaceGetPreferredFormat(surface, adapter),
                        // The images we want to draw may have transparency, so let's turn on alpha blending with over compositing (ɑ⋅foreground + (1-ɑ)⋅background).
                        // This will blend with whatever has already been drawn.
                        .blend = to_ptr(WGPUBlendState{
                        // Over blending for color
                        .color = {
                            .operation = WGPUBlendOperation_Add,
                            .srcFactor = WGPUBlendFactor_SrcAlpha,
                            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
                            },
                            // Leave destination alpha alone
                            .alpha = {
                                .operation = WGPUBlendOperation_Add,
                                .srcFactor = WGPUBlendFactor_Zero,
                                .dstFactor = WGPUBlendFactor_One
                                }
                            }),
                        .writeMask = WGPUColorWriteMask_All
                    }})
                })
            }));

        return true;
    }

    void GraphicsManager::Shutdown() {
        if (uniformBuffer) {
            wgpuBufferRelease(uniformBuffer);
            uniformBuffer = nullptr;
        }

        if (pipeline) {
            wgpuRenderPipelineRelease(pipeline);
            pipeline = nullptr;
        }

        if (shader_module) {
            wgpuShaderModuleRelease(shader_module);
            shader_module = nullptr;
        }


        if (instanceBuffer) {
            wgpuBufferRelease(instanceBuffer);
            instanceBuffer = nullptr;
        }

        if (vertexBuffer) {
            wgpuBufferRelease(vertexBuffer);
            vertexBuffer = nullptr;
        }
        
        if (queue) {
            wgpuQueueRelease(queue);
            queue = nullptr;
        }

        if (device) {
            wgpuDeviceRelease(device);
            device = nullptr;
        }

        if (adapter) {
            wgpuAdapterRelease(adapter);
            adapter = nullptr;
        }

        if (surface) {
            wgpuSurfaceRelease(surface);
            surface = nullptr;
        }

        if (instance) {
            wgpuInstanceRelease(instance);
            instance = nullptr;
        }

        for (auto& [_, tex] : textures) {
            if (tex.view) wgpuTextureViewRelease(tex.view);
            if (tex.sampler) wgpuSamplerRelease(tex.sampler);
            if (tex.texture) wgpuTextureRelease(tex.texture);
        }
        textures.clear();

        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        glfwTerminate();
        spdlog::info("GLFW shutdown complete");
    }

    bool GraphicsManager::LoadTexture(const std::string& name, const std::string& path) {
        if (textures.contains(name)) {
            spdlog::warn("Texture '{}' already loaded.", name);
            return true;
        }

        int width, height, channels;
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data) {
            spdlog::error("Failed to load image from '{}'", path);
            return false;
        }

        //create texture
        WGPUTexture tex = wgpuDeviceCreateTexture(device, to_ptr(WGPUTextureDescriptor{
            .label = WGPUStringView(path.c_str(), WGPU_STRLEN),
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
            .dimension = WGPUTextureDimension_2D,
            .size = { (uint32_t)width, (uint32_t)height, 1 },
            .format = WGPUTextureFormat_RGBA8UnormSrgb,
            .mipLevelCount = 1,
            .sampleCount = 1
            }));

        //upload image data to GPU
        wgpuQueueWriteTexture(
            queue,
            to_ptr<WGPUTexelCopyTextureInfo>({
                .texture = tex,
                .origin = { 0, 0, 0 }
                }),
            data,
            static_cast<size_t>(width * height * 4),
            to_ptr<WGPUTexelCopyBufferLayout>({
                .bytesPerRow = static_cast<uint32_t>(width * 4),
                .rowsPerImage = static_cast<uint32_t>(height)
                }),
            to_ptr(WGPUExtent3D{
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .depthOrArrayLayers = 1
                })
        );

        stbi_image_free(data);  //free memory when done

        //create view and sampler
        WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);

        WGPUSampler sampler = wgpuDeviceCreateSampler(device, to_ptr(WGPUSamplerDescriptor{
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Linear,
            .minFilter = WGPUFilterMode_Linear,
            .maxAnisotropy = 1
            }));

        // store texture data
        textures[name] = Texture{
            .texture = tex,
            .view = view,
            .sampler = sampler,
            .width = width,
            .height = height
        };

        spdlog::info("Loaded texture '{}' from '{}'", name, path);
        return true;
    }

    void GraphicsManager::Draw(const std::vector<Sprite>& sprites) {
        if (sprites.empty()) return;

        //create command encoder
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);

        //get the window surface texture view
        WGPUSurfaceTexture surface_texture{};
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
        WGPUTextureView current_texture_view = wgpuTextureCreateView(surface_texture.texture, nullptr);

        //clear screen and begin render pass
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, to_ptr(WGPURenderPassDescriptor({
            .colorAttachmentCount = 1,
            .colorAttachments = to_ptr<WGPURenderPassColorAttachment>({{
                .view = current_texture_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = WGPUColor{ 0.1, 0.1, 0.1, 1.0 }  //background color
            }})
            })));

        //compute and upload projection matrix
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        //testing
        spdlog::info("Framebuffer size: {}x{}", width, height);
        for (const Sprite& s : sprites) {
            spdlog::info("Drawing sprite at pixel position ({}, {}), scale: ({}, {})",
                s.position.x, s.position.y, s.scale.x, s.scale.y);
        }


        glm::mat4 projection = glm::ortho(0.0f, float(width), float(height), 0.0f, -1.0f, 1.0f);
        Uniforms uniforms{ projection };
        wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uniforms, sizeof(Uniforms));

        //set pipeline
        wgpuRenderPassEncoderSetPipeline(render_pass, pipeline);

        //set vertex buffer
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertexBuffer, 0, sizeof(vertices));

        //sort sprites back to front by z value
        std::vector<std::pair<Sprite, size_t>> sorted_sprites;
        for (size_t i = 0; i < sprites.size(); ++i) {
            sorted_sprites.emplace_back(sprites[i], i);
        }
        std::sort(sorted_sprites.begin(), sorted_sprites.end(),
            [](const auto& a, const auto& b) { return a.first.z < b.first.z; });

        //create and fill instance buffer
        std::vector<InstanceData> instances(sprites.size());
        for (size_t i = 0; i < sorted_sprites.size(); ++i) {
            const Sprite& s = sorted_sprites[i].first;

            glm::vec2 finalScale = s.scale;
            const auto it = textures.find(s.texture_name);
            if (it != textures.end()) {
                const Texture& tex = it->second;
                finalScale = s.scale;
                //if (tex.width < tex.height) {
                //    finalScale *= glm::vec2(float(tex.width) / tex.height, 1.0f);
                //}
                //else {
                //    finalScale *= glm::vec2(1.0f, float(tex.height) / tex.width);
                //}
            }

            instances[i] = {
                .translation = glm::vec3(s.position, s.z),
                .scale = finalScale
            };
        }

        instanceBuffer = wgpuDeviceCreateBuffer(device, to_ptr(WGPUBufferDescriptor{
            .label = WGPUStringView("Instance Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = sizeof(InstanceData) * instances.size()
            }));
        wgpuQueueWriteBuffer(queue, instanceBuffer, 0, instances.data(), sizeof(InstanceData) * instances.size());

        //set instance buffer (slot 1)
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, instanceBuffer, 0, sizeof(InstanceData) * instances.size());

        //draw sprites
        std::string lastTexture = "";
        const Sprite& s0 = sorted_sprites[0].first;
        const Texture& tex = textures[s0.texture_name];

        WGPUBindGroupLayout layout = wgpuRenderPipelineGetBindGroupLayout(pipeline, 0);
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, to_ptr(WGPUBindGroupDescriptor{
            .layout = layout,
            .entryCount = 3,
            .entries = to_ptr<WGPUBindGroupEntry>({
                {.binding = 0, .buffer = uniformBuffer, .size = sizeof(Uniforms)},
                {.binding = 1, .sampler = tex.sampler},
                {.binding = 2, .textureView = tex.view}
            })
            }));
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, bindGroup, 0, nullptr);
        wgpuBindGroupLayoutRelease(layout);

        //draw all instances at once
        wgpuRenderPassEncoderDraw(render_pass, 4, sorted_sprites.size(), 0, 0);

        //end render pass
        wgpuRenderPassEncoderEnd(render_pass);

        //submit command buffer
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &commandBuffer);

        //testing
        spdlog::info("Submitting draw command buffer...");
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        spdlog::info("Draw submitted.");
        if (!commandBuffer) {
            spdlog::error("Command buffer was null before submit!");
        }

        //present the frame
        wgpuSurfacePresent(surface);

        //cleanup
        wgpuTextureViewRelease(current_texture_view);
        wgpuTextureRelease(surface_texture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(commandBuffer);
    }

}
