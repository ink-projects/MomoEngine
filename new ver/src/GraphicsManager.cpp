#define GLFW_INCLUDE_NONE  //doesn't include OpenGL
#include "GLFW/glfw3.h"
#include <glfw3webgpu.h>

#include "GraphicsManager.h"
#include "spdlog/spdlog.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


//helper functions for inline struct initialization
template< typename T > constexpr const T* to_ptr(const T& val) { return &val; }
template< typename T, std::size_t N > constexpr const T* to_ptr(const T(&& arr)[N]) { return arr; }

//replacement function for wgpuSurfaceGetPreferredFormat()
WGPUTextureFormat wgpuSurfaceGetPreferredFormat(WGPUSurface surface, WGPUAdapter adapter) {
    WGPUSurfaceCapabilities capabilities{};
    wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
    const WGPUTextureFormat result = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return result;
}

namespace {
    // A vertex buffer containing a textured square.
    const struct {
        float x, y; // position
        float u, v; // texcoords
    } vertices[] = {
        // Position       // Texcoords
        { -1.0f, -1.0f, 0.0f, 1.0f },  // bottom-left
        {  1.0f, -1.0f, 1.0f, 1.0f },  // bottom-right
        { -1.0f,  1.0f, 0.0f, 0.0f },  // top-left
        {  1.0f,  1.0f, 1.0f, 0.0f },  // top-right
    };

    struct InstanceData {
        glm::vec3 translation;
        glm::vec2 scale;
        // rotation?
    };

    struct Uniforms {
        glm::mat4 projection;
    };
}

namespace momoengine {

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

        glfwSetWindowAspectRatio(window, window_width, window_height);
        spdlog::info("Window created: {}x{}", window_width, window_height);

        // WebGPU Initializations
        //create instance
        instance = wgpuCreateInstance(to_ptr(WGPUInstanceDescriptor{}));
        if (!instance) {
            spdlog::error("Failed to create WebGPU instance.");
            glfwTerminate();
            return false;
        }
        spdlog::info("WebGPU instance created.");

        surface = glfwCreateWindowWGPUSurface(instance, window);

        adapter = nullptr;
        wgpuInstanceRequestAdapter(
            instance,
            to_ptr(WGPURequestAdapterOptions{ .featureLevel = WGPUFeatureLevel_Core, .compatibleSurface = surface }),
            WGPURequestAdapterCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* adapter_ptr, void*) {
                    if (status != WGPURequestAdapterStatus_Success) {
                        std::cerr << "Failed to get a WebGPU adapter: " << std::string_view(message.data, message.length) << std::endl;
                        glfwTerminate();
                    }

                    *static_cast<WGPUAdapter*>(adapter_ptr) = adapter;
                },
                .userdata1 = &(adapter)
            }
        );
        while (!adapter) wgpuInstanceProcessEvents(instance);
        assert(adapter);

        wgpuAdapterRequestDevice(
            adapter,
            to_ptr(WGPUDeviceDescriptor{
                // Add an error callback for more debug info
                .uncapturedErrorCallbackInfo = {.callback = [](WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void*, void*) {
                    std::cerr << "WebGPU uncaptured error type " << int(type) << " with message: " << std::string_view(message.data, message.length) << std::endl;
                }}
                }),
            WGPURequestDeviceCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* device_ptr, void*) {
                    if (status != WGPURequestDeviceStatus_Success) {
                        std::cerr << "Failed to get a WebGPU device: " << std::string_view(message.data, message.length) << std::endl;
                        glfwTerminate();
                    }

                    *static_cast<WGPUDevice*>(device_ptr) = device;
                },
                .userdata1 = &(device)
            }
        );
        while (!device) wgpuInstanceProcessEvents(instance);
        assert(device);

        queue = wgpuDeviceGetQueue(device);

        //surface configuration
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        wgpuSurfaceConfigure(surface, to_ptr(WGPUSurfaceConfiguration{
            .device = device,
            .format = wgpuSurfaceGetPreferredFormat(surface, adapter),
            .usage = WGPUTextureUsage_RenderAttachment,
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .presentMode = WGPUPresentMode_Fifo // Explicitly set this because of a Dawn bug
            }));

        //vertex buffer
        vertex_buffer = wgpuDeviceCreateBuffer(device, to_ptr(WGPUBufferDescriptor{
            .label = WGPUStringView("Vertex Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = sizeof(vertices)
         }));

        wgpuQueueWriteBuffer(queue, vertex_buffer, 0, vertices, sizeof(vertices));
        spdlog::info("Vertex buffer created and uploaded ({} bytes).", sizeof(vertices));

        //uniform buffer
        uniform_buffer = wgpuDeviceCreateBuffer(device, to_ptr(WGPUBufferDescriptor{
            .label = WGPUStringView("Uniform Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof(Uniforms)
        }));
        spdlog::info("Uniform buffer created ({} bytes).", sizeof(Uniforms));

        //sampler
        sampler = wgpuDeviceCreateSampler(device, to_ptr(WGPUSamplerDescriptor{
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Linear,
            .minFilter = WGPUFilterMode_Linear,
            .maxAnisotropy = 1
        }));
        spdlog::info("Sampler created.");

        //WGSL Shaders
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

        //create shader module from the source
        WGPUShaderSourceWGSL source_desc = {};
        source_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        source_desc.code = WGPUStringView(source, std::string_view(source).length());
        // Point to the code descriptor from the shader descriptor.
        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = &source_desc.chain;
        shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

        //pipeline
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

    bool GraphicsManager::LoadTexture(const std::string& name, const std::string& path) {
        //load the image pixels
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data) {
            spdlog::error("Failed to load texture: {}", path);
            return false;
        }
        spdlog::info("Loaded texture '{}' ({}x{}, {} channels)", path, width, height, channels);

        //create the texture
        WGPUTexture tex = wgpuDeviceCreateTexture(device, to_ptr(WGPUTextureDescriptor{
            .label = WGPUStringView(path.c_str(), WGPU_STRLEN),
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
            .dimension = WGPUTextureDimension_2D,
            .size = { (uint32_t)width, (uint32_t)height, 1 },
            .format = WGPUTextureFormat_RGBA8UnormSrgb,
            .mipLevelCount = 1,
            .sampleCount = 1
         }));

        //upload pixels
        wgpuQueueWriteTexture(
            queue,
            to_ptr<WGPUTexelCopyTextureInfo>({ .texture = tex }),
            data,
            width * height * 4,
            to_ptr<WGPUTexelCopyBufferLayout>({ .bytesPerRow = (uint32_t)(width * 4), .rowsPerImage = (uint32_t)height }),
            to_ptr(WGPUExtent3D{ (uint32_t)width, (uint32_t)height, 1 })
        );

        //free image memory
        stbi_image_free(data);

        return true;
    }

    void GraphicsManager::Draw(const std::vector<Sprite>& sprites) {
        if (sprites.empty()) return;

        //create projection matrix
        Uniforms uniforms;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        uniforms.projection = glm::mat4(1.0f);
        uniforms.projection[0][0] = uniforms.projection[1][1] = 1.0f / 100.0f;

        if (width < height) {
            uniforms.projection[1][1] *= (float)width / (float)height;
        }
        else {
            uniforms.projection[0][0] *= (float)height / (float)width;
        }

        wgpuQueueWriteBuffer(queue, uniform_buffer, 0, &uniforms, sizeof(Uniforms));

        //sort the sprites from back to front
        std::vector<Sprite> sorted = sprites;
        std::sort(sorted.begin(), sorted.end(),
            [](const Sprite& a, const Sprite& b) { return a.z < b.z; });

        //upload instance data
        std::vector<InstanceData> instances(sorted.size());
        for (size_t i = 0; i < sorted.size(); ++i) {
            instances[i].translation = sorted[i].position;
            instances[i].scale = sorted[i].scale;
        }

        //instance buffer to fill
        instance_buffer = wgpuDeviceCreateBuffer(device, to_ptr<WGPUBufferDescriptor>({
            .label = WGPUStringView("Instance Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = sizeof(InstanceData) * sprites.size()
        }));
        if (instances.size() > 0) {
            wgpuQueueWriteBuffer(queue, instance_buffer, 0,
                instances.data(), sizeof(InstanceData) * instances.size());
        }

        //create command encoder
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);

        //get surface texture to draw to
        WGPUSurfaceTexture surface_texture{};
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
        WGPUTextureView current_texture_view = wgpuTextureCreateView(surface_texture.texture, nullptr);

        //start the render pass
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, to_ptr<WGPURenderPassDescriptor>({
            .colorAttachmentCount = 1,
            .colorAttachments = to_ptr<WGPURenderPassColorAttachment>({{
                .view = current_texture_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED, // not a 3D texture
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                // Choose the background color.
                .clearValue = WGPUColor{ 0.1, 0.1, 0.2, 1.0 }
            }})
        }));

        //set the pipeline
        wgpuRenderPassEncoderSetPipeline(render_pass, pipeline);

        //attach vertex data for the quad as slot 0
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0 /* slot */, vertex_buffer, 0, 4 * 4 * sizeof(float));

        //attach instance data as slot 1
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1 /* slot */, instance_buffer, 0, sizeof(InstanceData) * instances.size());

        //draw the sprites
        for (size_t i = 0; i < sorted.size(); ++i) {
            // TODO: bind sprite texture here once bind groups are ready
            wgpuRenderPassEncoderDraw(render_pass, 4, 1, 0, i);
        }

        //end render pass
        wgpuRenderPassEncoderEnd(render_pass);

        //finish and submit commands
        WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &command_buffer);

        //present the new frame
        wgpuSurfacePresent(surface);

        //cleanup
        wgpuTextureViewRelease(current_texture_view);
        wgpuCommandBufferRelease(command_buffer);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureRelease(surface_texture.texture);
        wgpuBufferRelease(instance_buffer);
    }

    void GraphicsManager::Shutdown() {
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
        spdlog::info("GLFW shutdown complete");
    }

}
