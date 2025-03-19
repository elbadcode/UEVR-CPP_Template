#include "uevr/API.hpp"
#include "../imgui/imgui_impl_dx11.h"

#include "d3d11.hpp"

D3D11 g_d3d11{};

bool D3D11::initialize() {
    const auto renderer_data = uevr::API::get()->param()->renderer;
    auto swapchain = (IDXGISwapChain*)renderer_data->swapchain;
    auto device = (ID3D11Device*)renderer_data->device;

    // Get back buffer.
    ComPtr<ID3D11Texture2D> backbuffer{};

    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)))) {
        return false;
    }

    // Create a render target view of the back buffer.
    if (FAILED(device->CreateRenderTargetView(backbuffer.Get(), nullptr, &this->bb_rtv))) {
        return false;
    }

    // Get backbuffer description.
    D3D11_TEXTURE2D_DESC backbuffer_desc{};

    backbuffer->GetDesc(&backbuffer_desc);

    backbuffer_desc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // Create our render target.
    if (FAILED(device->CreateTexture2D(&backbuffer_desc, nullptr, &this->rt))) {
        return false;
    }

    // Create our render target view.
    if (FAILED(device->CreateRenderTargetView(this->rt.Get(), nullptr, &this->rt_rtv))) {
        return false;
    }

    // Create our render target shader resource view.
    if (FAILED(device->CreateShaderResourceView(this->rt.Get(), nullptr, &this->rt_srv))) {
        return false;
    }

    this->rt_width = backbuffer_desc.Width;
    this->rt_height = backbuffer_desc.Height;

    ComPtr<ID3D11DeviceContext> context{};

    device->GetImmediateContext(&context);

    if (!ImGui_ImplDX11_Init(device, context.Get())) {
        return false;
    }

    return true;
}

void D3D11::render_imgui() {
    auto draw_data = ImGui::GetDrawData();

    if (draw_data == nullptr) {
        return;
    }

    ComPtr<ID3D11DeviceContext> context{};
    float clear_color[]{0.0f, 0.0f, 0.0f, 0.0f};

    const auto renderer_data = uevr::API::get()->param()->renderer;
    auto device = (ID3D11Device*)renderer_data->device;
    device->GetImmediateContext(&context);
    context->ClearRenderTargetView(this->rt_rtv.Get(), clear_color);
    context->OMSetRenderTargets(1, this->rt_rtv.GetAddressOf(), NULL);
    ImGui_ImplDX11_RenderDrawData(draw_data);

    // Set the back buffer to be the render target.
    context->OMSetRenderTargets(1, this->bb_rtv.GetAddressOf(), nullptr);
    ImGui_ImplDX11_RenderDrawData(draw_data);
}

void D3D11::render_imgui_vr(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) {
    if (context == nullptr || rtv == nullptr) {
        return;
    }

    auto draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr) {
        return;
    }

    context->OMSetRenderTargets(1, &rtv, NULL);
    ImGui_ImplDX11_RenderDrawData(draw_data);
}

// #include "../imgui/imgui_impl_dx11.h"
// #include "uevr/API.hpp"

// #include "../reshade/reshade.hpp"

// #include "d3d11.hpp"
// #include "filesystem"
// D3D11 g_d3d11{};
// reshade::api::effect_runtime *out_runtime = nullptr;
// bool D3D11::initialize() {

// // typedef struct {
// //     int renderer_type;
// //     void* device;
// //     void* swapchain;
// //     void* command_queue;
// // } UEVR_RendererData;



//     const auto renderer_data = uevr::API::get()->param()->renderer;
//     auto swapchain = (IDXGISwapChain*)renderer_data->swapchain;
//     auto device = (ID3D11Device*)renderer_data->device;

//     reshade::api::device_api api = reshade::api::device_api::d3d11;
//     auto reshade_device = (void*)device;
//     auto reshade_command_queue = (void*)renderer_data->command_queue;
//     auto reshade_swapchain = (void*)swapchain;


//     auto config_path = L"";
//     wchar_t reshade_dll_path[MAX_PATH];
//     GetModuleFileNameW(GetModuleHandleW(L"ReShade64.dll"), reshade_dll_path, MAX_PATH);
//     config_path = std::filesystem::path(reshade_dll_path).replace_extension(L".ini").c_str();
    
    
//   if(!reshade::create_effect_runtime(api, reshade_device, reshade_command_queue, reshade_swapchain, config_path, &out_runtime))
//    return false;
//     /// Creates a new effect runtime for an existing swapchain, for when it was not already hooked by ReShade (e.g. because the RESHADE_DISABLE_GRAPHICS_HOOK environment variable is set).
// 	/// </summary>
// 	/// <param name="api">Underlying graphics API used.</param>
// 	/// <param name="device">'IDirect3DDevice9', 'ID3D10Device', 'ID3D11Device', 'ID3D12Device', 'HGLRC' or 'VkDevice', depending on the graphics API.</param>
// 	/// <param name="command_queue">'ID3D11DeviceContext', 'ID3D12CommandQueue', or 'VkQueue', depending on the graphics API.</param>
// 	/// <param name="swapchain">'IDirect3DSwapChain9', 'IDXGISwapChain', 'HDC' or 'VkSwapchainKHR', depending on the graphics API.</param>
// 	/// <param name="config_path">Path to the configuration file the effect runtime should use.</param>
// 	/// <param name="out_runtime">Pointer to a variable that is set to the pointer of the created effect runtime.</param>
// 	/// <returns><see langword="true"/> if the effect_runtime was successfully created, <see langword="false"/> otherwise (in this case <paramref name="out_runtime"/> is set to <see langword="nullptr"/>).</returns>
// 	///inline bool create_effect_runtime(, , void *command_queue, void *swapchain, const char *config_path, reshade::api::effect_runtime **out_runtime)


// 	/// <summary>

//     // Get back buffer.
//     ComPtr<ID3D11Texture2D> backbuffer{};

//     if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)))) {
//         return false;
//     }
//     auto backbuffer_count = out_runtime->get_back_buffer_count();
//     int curr_bb_idx = 0;
//     reshade::api::resource back_buffer = out_runtime->get_back_buffer(curr_bb_idx);
  
//     // Create a render target view of the back buffer.
//     if (FAILED(device->CreateRenderTargetView(backbuffer.Get(), nullptr, &this->bb_rtv))) {
//         return false;
//     }

//     // Get backbuffer description.
//     D3D11_TEXTURE2D_DESC backbuffer_desc{};

//     backbuffer->GetDesc(&backbuffer_desc);

//     backbuffer_desc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

//     // Create our render target.
//     if (FAILED(device->CreateTexture2D(&backbuffer_desc, nullptr, &this->rt))) {
//         return false;
//     }

//     // Create our render target view.
//     if (FAILED(device->CreateRenderTargetView(this->rt.Get(), nullptr, &this->rt_rtv))) {
//         return false;
//     }

//     // Create our render target shader resource view.
//     if (FAILED(device->CreateShaderResourceView(this->rt.Get(), nullptr, &this->rt_srv))) {
//         return false;
//     }

//     this->rt_width = backbuffer_desc.Width;
//     this->rt_height = backbuffer_desc.Height;

//     ComPtr<ID3D11DeviceContext> context{};

//     device->GetImmediateContext(&context);

//     if (!ImGui_ImplDX11_Init(device, context.Get())) {
//         return false;
//     }
//     return true;
// }

// void D3D11::render_imgui() {
//     auto draw_data = ImGui::GetDrawData();

//     if (draw_data == nullptr) {
//         return;
//     }

//     ComPtr<ID3D11DeviceContext> context{};
//     float clear_color[]{0.0f, 0.0f, 0.0f, 0.0f};

//     const auto renderer_data = uevr::API::get()->param()->renderer;
//     auto device = (ID3D11Device*)renderer_data->device;
//     device->GetImmediateContext(&context);
//     context->ClearRenderTargetView(this->rt_rtv.Get(), clear_color);
//     context->OMSetRenderTargets(1, this->rt_rtv.GetAddressOf(), NULL);
//     ImGui_ImplDX11_RenderDrawData(draw_data);

//     // Set the back buffer to be the render target.
//     context->OMSetRenderTargets(1, this->bb_rtv.GetAddressOf(), nullptr);
//     ImGui_ImplDX11_RenderDrawData(draw_data);
// }

// void D3D11::render_imgui_vr(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) {
//     if (context == nullptr || rtv == nullptr) {
//         return;
//     }

//     auto draw_data = ImGui::GetDrawData();
//     if (draw_data == nullptr) {
//         return;
//     }

//     context->OMSetRenderTargets(1, &rtv, NULL);
//     ImGui_ImplDX11_RenderDrawData(draw_data);
//     reshade::update_and_present_effect_runtime(out_runtime);
// }



