/*
This file (Plugin.cpp) is licensed under the MIT license and is separate from the rest of the UEVR codebase.

Copyright (c) 2023 praydog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <sstream>
#include <mutex>
#include <memory>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <Windows.h>
#include "renderlib/imgui/imgui.h"

// only really necessary if you want to render to the screen
#include "renderlib/uevr_imgui/imgui_impl_dx11.h"
#include "renderlib/uevr_imgui/imgui_impl_dx12.h"
#include "renderlib/uevr_imgui/imgui_impl_win32.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>
#include "renderlib/rendering/d3d11.hpp"
#include "renderlib/rendering/d3d12.hpp"

#include "uevr/Plugin.hpp"

using namespace uevr;

#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }

class ImGuiTemplate : public uevr::Plugin {
public:
	ImGuiTemplate() = default;

	void on_dllmain(HMODULE hmod) override {}

	void on_initialize() override {
		ImGui::CreateContext();
	}

	void on_present() override {
		std::scoped_lock _{ m_imgui_mutex };

		if (!m_initialized) {
			if (!initialize_imgui()) {
				API::get()->log_info("Failed to initialize imgui");
				return;
			}
			else {
				API::get()->log_info("Initialized imgui");
			}
		}

		const auto renderer_data = API::get()->param()->renderer;

		if (!API::get()->param()->vr->is_hmd_active()) {
			if (!m_was_rendering_desktop) {
				m_was_rendering_desktop = true;
				on_device_reset();
				return;
			}

			m_was_rendering_desktop = true;

			if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
				ImGui_ImplDX11_NewFrame();
				g_d3d11.render_imgui();
			}
			else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
				auto command_queue = (ID3D12CommandQueue*)renderer_data->command_queue;

				if (command_queue == nullptr) {
					return;
				}

				ImGui_ImplDX12_NewFrame();
				g_d3d12.render_imgui();
			}
		}
		if (GetAsyncKeyState(VK_F2)){
			m_display_menu = !m_display_menu;
		}

	}


	void reset_height() {
		auto& api = API::get();
		auto vr = api->param()->vr;
		UEVR_Vector3f origin{};
		vr->get_standing_origin(&origin);

		UEVR_Vector3f hmd_pos{};
		UEVR_Quaternionf hmd_rot{};
		vr->get_pose(vr->get_hmd_index(), &hmd_pos, &hmd_rot);

		origin.y = hmd_pos.y;

		vr->set_standing_origin(&origin);
	}

	void on_device_reset() override {
		PLUGIN_LOG_ONCE("Example Device Reset");

		std::scoped_lock _{ m_imgui_mutex };

		const auto renderer_data = API::get()->param()->renderer;

		if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
			ImGui_ImplDX11_Shutdown();
			g_d3d11 = {};
		}

		if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
			g_d3d12.reset();
			ImGui_ImplDX12_Shutdown();
			g_d3d12 = {};
		}

		m_initialized = false;
	}

	void on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) override {
		PLUGIN_LOG_ONCE("Post Render VR Framework DX11");

		const auto vr_active = API::get()->param()->vr->is_hmd_active();

		if (!m_initialized || !vr_active) {
			return;
		}

		if (m_was_rendering_desktop) {
			m_was_rendering_desktop = false;
			on_device_reset();
			return;
		}

		std::scoped_lock _{ m_imgui_mutex };

		ImGui_ImplDX11_NewFrame();
		g_d3d11.render_imgui_vr(context, rtv);
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

		std::scoped_lock _{ m_imgui_mutex };

		ImGui_ImplDX12_NewFrame();
		g_d3d12.render_imgui_vr(command_list, rtv);
	}

	bool on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		return !ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard;
	}
	
	void on_custom_event(const char* event_name, const char* event_data) override {
		API::get()->log_info("Custom Event: %s %s", event_name, event_data);
	}

	void dispatch_lua_event(const char* event_name, const char* event_data) override {
		API::get()->dispatch_lua_event(event_name, event_data);
	}

	
	void print_all_objects() {
		API::get()->log_info("Printing all objects...");

		API::get()->log_info("Chunked: %i", API::FUObjectArray::is_chunked());
		API::get()->log_info("Inlined: %i", API::FUObjectArray::is_inlined());
		API::get()->log_info("Objects offset: %i", API::FUObjectArray::get_objects_offset());
		API::get()->log_info("Item distance: %i", API::FUObjectArray::get_item_distance());
		API::get()->log_info("Object count: %i", API::FUObjectArray::get()->get_object_count());

		const auto objects = API::FUObjectArray::get();

		if (objects == nullptr) {
			API::get()->log_error("Failed to get FUObjectArray");
			return;
		}

		for (int32_t i = 0; i < objects->get_object_count(); ++i) {
			const auto object = objects->get_object(i);

			if (object == nullptr) {
				continue;
			}

			const auto name = object->get_full_name();

			if (name.empty()) {
				continue;
			}

			std::string name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(name) };

			API::get()->log_info(" [%d]: %s", i, name_narrow.c_str());
		}
	}


	void test_console_manager() {
		const auto console_manager = API::get()->get_console_manager();

		if (console_manager != nullptr) {
			API::get()->log_info("Console manager @ 0x%p", console_manager);
			const auto& objects = console_manager->get_console_objects();

			for (const auto& object : objects) {
				if (object.key != nullptr) {
					// convert from wide to narrow string (we do not have utility::narrow in this context).
					std::string key_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(object.key) };
					if (object.value != nullptr) {
						const auto command = object.value->as_command();

						if (command != nullptr) {
							API::get()->log_info(" Console COMMAND: %s @ 0x%p", key_narrow.c_str(), object.value);
						}
						else {
							API::get()->log_info(" Console VARIABLE: %s @ 0x%p", key_narrow.c_str(), object.value);
						}
					}
				}
			}

			auto cvar = console_manager->find_variable(L"r.Color.Min");

			if (cvar != nullptr) {
				API::get()->log_info("Found r.Color.Min @ 0x%p (%f)", cvar, cvar->get_float());
			}
			else {
				API::get()->log_error("Failed to find r.Color.Min");
			}

			auto cvar2 = console_manager->find_variable(L"r.Upscale.Quality");

			if (cvar2 != nullptr) {
				API::get()->log_info("Found r.Upscale.Quality @ 0x%p (%d)", cvar2, cvar2->get_int());
				cvar2->set(cvar2->get_int() + 1);
			}
			else {
				API::get()->log_error("Failed to find r.Upscale.Quality");
			}
		}
		else {
			API::get()->log_error("Failed to find console manager");
		}
	}

	void test_engine(API::UGameEngine* engine) {
		if (engine == nullptr)
			return;
		// Log the UEngine name.
		const auto uengine_name = engine->get_full_name();

		// Convert from wide to narrow string (we do not have utility::narrow in this context).
		std::string uengine_name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(uengine_name) };

		API::get()->log_info("Engine name: %s", uengine_name_narrow.c_str());

		// Test if we can dcast to UObject.
		{
			const auto engine_as_object = engine->dcast<API::UObject>();

			if (engine != nullptr) {
				API::get()->log_info("Engine successfully dcast to UObject");
			}
			else {
				API::get()->log_error("Failed to dcast Engine to UObject");
			}
		}

		// Go through all of engine's fields and log their names.
		const auto engine_class_ours = (API::UStruct*)engine->get_class();
		for (auto super = engine_class_ours; super != nullptr; super = super->get_super()) {
			for (auto field = super->get_child_properties(); field != nullptr; field = field->get_next()) {
				const auto field_fname = field->get_fname();
				const auto field_name = field_fname->to_string();
				const auto field_class = field->get_class();

				std::wstring prepend{};

				if (field_class != nullptr) {
					const auto field_class_fname = field_class->get_fname();
					const auto field_class_name = field_class_fname->to_string();

					prepend = field_class_name + L" ";
				}

				// Convert from wide to narrow string (we do not have utility::narrow in this context).
				std::string field_name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(prepend + field_name) };
				API::get()->log_info(" Field name: %s", field_name_narrow.c_str());
			}
		}

		// Check if we can find the GameInstance and call is_a() on it.
		const auto game_instance = engine->get_property<API::UObject*>(L"GameInstance");

		if (game_instance != nullptr) {
			const auto game_instance_class = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.GameInstance");

			if (game_instance->is_a(game_instance_class)) {
				const auto& local_players = game_instance->get_property<API::TArray<API::UObject*>>(L"LocalPlayers");

				if (local_players.count > 0 && local_players.data != nullptr) {
					const auto local_player = local_players.data[0];


				}
				else {
					API::get()->log_error("Failed to find LocalPlayers");
				}

				API::get()->log_info("GameInstance is a UGameInstance");
			}
			else {
				API::get()->log_error("GameInstance is not a UGameInstance");
			}
		}
		else {
			API::get()->log_error("Failed to find GameInstance");
		}

		// Find the Engine object and compare it to the one we have.
		const auto engine_class = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.GameEngine");
		if (engine_class != nullptr) {
			// Round 1, check if we can find it via get_first_object_by_class.
			const auto engine_searched = engine_class->get_first_object_matching<API::UGameEngine>(false);

			if (engine_searched != nullptr) {
				if (engine_searched == engine) {
					API::get()->log_info("Found Engine object @ 0x%p", engine_searched);
				}
				else {
					API::get()->log_error("Found Engine object @ 0x%p, but it's not the same as the one we have", engine_searched);
				}
			}
			else {
				API::get()->log_error("Failed to find Engine object");
			}

			// Round 2, check if we can find it via get_objects_by_class.
			const auto objects = engine_class->get_objects_matching<API::UGameEngine>(false);

			if (!objects.empty()) {
				for (const auto& obj : objects) {
					if (obj == engine) {
						API::get()->log_info("Found Engine object @ 0x%p", obj);
					}
					else {
						API::get()->log_info("Found unrelated Engine object @ 0x%p", obj);
					}
				}
			}
			else {
				API::get()->log_error("Failed to find Engine objects");
			}
		}
		else {
			API::get()->log_error("Failed to find Engine class");
		}
	}

	void test_mesh_attachment() {
		struct {
			API::UClass* c;
			API::TArray<API::UObject*> return_value{};
		} component_params;

		component_params.c = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.SkeletalMeshComponent");
		const auto pawn = API::get()->get_local_pawn(0);

		if (component_params.c != nullptr && pawn != nullptr) {
			// either or.
			pawn->call_function(L"K2_GetComponentsByClass", &component_params);
			pawn->call_function(L"GetComponentsByClass", &component_params);

			if (component_params.return_value.empty()) {
				API::get()->log_error("Failed to find any SkeletalMeshComponents");
			}

			for (auto mesh : component_params.return_value) {
				auto state = API::UObjectHook::get_or_add_motion_controller_state(mesh);
			}
		}
		else {
			API::get()->log_error("Failed to find SkeletalMeshComponent class or local pawn");
		}
	}


	void on_pre_engine_tick(API::UGameEngine* engine, float delta) override {
		PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);

		static bool once = true;

		// Unit tests for the API basically.
		if (once) {
			once = false;

			API::get()->log_info("Running once on pre engine tick");
			API::get()->execute_command(L"stat fps");

			API::FName test_name{ L"Left" };
			std::string name_narrow{ std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(test_name.to_string()) };
			API::get()->log_info("Test FName: %s", name_narrow.c_str());

			print_all_objects();
			test_console_manager();
			test_engine(engine);
		}

		if (m_initialized) {
			std::scoped_lock _{ m_imgui_mutex };

			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			internal_frame();

			ImGui::EndFrame();
			ImGui::Render();
		}
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

		auto rotationd = (UEVR_Rotatord*)rotation;

		// Decoupled pitch.
		if (!is_double) {
			rotation->pitch = 0.0f;
		}
		else {
			rotationd->pitch = 0.0;
		}
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

private:
	bool initialize_imgui() {
		if (m_initialized) {
			return true;
		}

		std::scoped_lock _{ m_imgui_mutex };

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		static const auto imgui_ini = API::get()->get_persistent_dir(L"imgui_template_plugin.ini").string();
		ImGui::GetIO().IniFilename = imgui_ini.c_str();

		const auto renderer_data = API::get()->param()->renderer;

		DXGI_SWAP_CHAIN_DESC swap_desc{};
		auto swapchain = (IDXGISwapChain*)renderer_data->swapchain;
		swapchain->GetDesc(&swap_desc);

		m_wnd = swap_desc.OutputWindow;

		if (!ImGui_ImplWin32_Init(m_wnd)) {
			return false;
		}

		if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
			if (!g_d3d11.initialize()) {
				return false;
			}
		}
		else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
			if (!g_d3d12.initialize()) {
				return false;
			}
		}

		m_initialized = true;
		return true;
	}

	void internal_frame() {
		if (!m_display_menu) return;
		if(m_display_menu) {
		if (ImGui::Begin("Very Cool Template Plugin")) {

			ImGui::Text("Hello from the very cool plugin!");
			ImGui::Text("Decoupled pitch: %i", API::VR::is_decoupled_pitch_enabled());
			if (ImGui::Button("Toggle decoupled pitch")) {
				API::VR::set_decoupled_pitch_enabled(!API::VR::is_decoupled_pitch_enabled());
			}
			
			ImGui::PopID();
			std::wstring object_path;
			static char obj_path[256]{};
			ImGui::Text("Enter an object to spawn");
			ImGui::SameLine();
			if (ImGui::InputText("objectspawner", obj_path, sizeof(obj_path)))
				object_path = widen(obj_path);
		
			if (ImGui::Button("Spawn Object")) {
				auto obj_res = API::get()->find_uobject<API::UObject>(object_path);
				struct {
					API::UObject* o{};
					bool return_value{ false };
				} params;
				params.o = obj_res;
				k2_sys(L"IsValid", &params);
				if (params.return_value)
					auto spawned_component = API::get()->spawn_object(obj_res->get_class(), obj_res->get_outer());
			}

			static char lua_event_name[256]{};
			ImGui::Text("Send Event to Lua");
			ImGui::InputText("LuaEvent", lua_event_name, sizeof(lua_event_name));
			
			static char lua_event_data[256]{};
			ImGui::InputText("LuaData", lua_event_data, sizeof(lua_event_data));
			

			if (ImGui::Button("Send Lua Event")) {
				if (lua_event_name[0] != '\0') {
					dispatch_lua_event(lua_event_name, lua_event_data);
				}
			}
			static char input[256]{};
			std::string mod_value = API::VR::get_mod_value<std::string>(input);
			ImGui::Text("Mod value: %s", mod_value.c_str());

			if (ImGui::Button("Save Config")) {
				API::VR::save_config();
			}
			

			if (ImGui::Button("Reload Config")) {
				API::VR::reload_config();
			}

			if (ImGui::Button("Toggle UObjectHook disabled")) {
				const auto value = API::UObjectHook::is_disabled();

				API::UObjectHook::set_disabled(!value);
			}
		}	
		ImGui::End();
		}
	}

private:
	HWND m_wnd{};
	bool m_initialized{ false };
	bool m_was_rendering_desktop{ false };
	bool m_display_menu = true;
	std::recursive_mutex m_imgui_mutex{};
	ImGuiKey m_toggle_key = ImGuiKey_F2;
};

// Actually creates the plugin. Very important that this global is created.
// The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
std::unique_ptr<ImGuiTemplate> g_plugin{ new ImGuiTemplate() };
