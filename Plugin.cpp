#include <Windows.h>
#include "include/uevr/Plugin.hpp"

using namespace uevr;

// Fully barebones plugin template. Use this if you don't need to draw to screen directly
// Even if you decide later you want imgui functions we are working out a way to do that by proxy



#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }

class TemplatePlugin : public uevr::Plugin {
public:
    TemplatePlugin() = default;

    void on_dllmain() override {

    }

    void on_initialize() override {
    }

    void on_present() override {
     
    }


    void on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) override {
        PLUGIN_LOG_ONCE("Post Render VR Framework DX11");

        const auto vr_active = API::get()->param()->vr->is_hmd_active();

    }

    void on_post_render_vr_framework_dx12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv) override {
        PLUGIN_LOG_ONCE("Post Render VR Framework DX12");

        const auto vr_active = API::get()->param()->vr->is_hmd_active();

        if (!m_initialized || !vr_active) {
            return;
        }

        if (m_was_rendering_desktop) {
            m_was_rendering_desktop = false;
            on_device_reset();
            return;
        }
    }

    bool on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override { 
    }

    void on_pre_engine_tick(API::UGameEngine* engine, float delta) override {
        PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);

    }

    void on_post_engine_tick(API::UGameEngine* engine, float delta) override {
        PLUGIN_LOG_ONCE("Post Engine Tick: %f", delta);
    }

    void on_pre_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override {
        PLUGIN_LOG_ONCE("Pre Slate Draw Window");
    }

    void on_post_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override {
        PLUGIN_LOG_ONCE("Post Slate Draw Window");
    }

    void on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters, 
                                             UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double) override
    {
        PLUGIN_LOG_ONCE("Pre Calculate Stereo View Offset");

    }

    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters, 
                                              UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
    {
        PLUGIN_LOG_ONCE("Post Calculate Stereo View Offset");
    }

    void on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
        PLUGIN_LOG_ONCE("Pre Viewport Client Draw");
    }

    void on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
        PLUGIN_LOG_ONCE("Post Viewport Client Draw");
    }


// Actually creates the plugin. Very important that this global is created.
// The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
std::unique_ptr<TemplatePlugin> g_plugin{new TemplatePlugin()};

