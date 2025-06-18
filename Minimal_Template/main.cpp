// You probably don't need all of these so let copilot check for you
// These are included to preserve a working order for compilation, 
// some include orders can cause errors that copilot doesn't know how to solve
#include <stdio.h>
#include <string.h>
#include <memory>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <cctype>
#include <filesystem>

#include <sstream>
#include <mutex>

#include <Windows.h>

// Using praydog's provided Plugin.hpp interface for simplicity with C++
// API can be easily accessed through uevr::API::get()-> with this header
// You may need to use the C api if you want to setup bindings to another language
#include "uevr/Plugin.hpp"

using namespace uevr;

#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }
    
    class TemplatePlugin : public uevr::Plugin {
    public:
        API::UGameEngine* m_UEngine;
        const UEVR_PluginInitializeParam* m_Param;
        const UEVR_VRData* m_VR;
        int m_DumpCount;
        std::wstring m_PersistentDir;

        TemplatePlugin() = default;

		// This will spawn you into dllmain on attach event
		// UEVR's pluginloader calls dllmain overrides as early as possible
		// so if you have anything you want to run before VR even starts up you can do it through here
		
        void on_dllmain(HMODULE hmod) override {
            m_DumpCount = 0;
        }

		// doesn't really do anything, this is to show that you can handle messages from the window, 
		// e.g. if you want to handle frontend messages
		bool on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
			return UpdateWindow(hwnd);
		}

		void on_custom_event(const char* event_name, const char* event_data) override {
			API::get()->log_info("Custom Event: %s %s", event_name, event_data);
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


		void dispatch_lua_event(const char* event_name, const char* event_data) override {
			API::get()->dispatch_lua_event(event_name, event_data);
		}

		void dispatch_lua_event(std::string event_name, std::string event_data) {
			dispatch_lua_event(event_name.data(), event_data.data()); 
		}


        void on_initialize() override {
            wchar_t PersistentDir[MAX_PATH];
            m_Param = API::get()->param();
            m_VR = m_Param->vr;
            API::get()->param()->functions->get_persistent_dir(PersistentDir, MAX_PATH);
			m_PersistentDir = std::filesystem::path(PersistentDir);
        }


		// Test attaching skeletal mesh components with UObjectHook.
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



        void on_present() {
            static bool Dumped = false;


            if (Dumped == true) return;

            if (GetAsyncKeyState(VK_F8) & 1)
            {
                Dumped = true;
                API::get()->log_info("dump.dll: dumping values");
                print_all_objects();
            }
        }


        void print_all_objects() {
            m_DumpCount++;
            std::wstring File = m_PersistentDir +L"\\object_dump_" + std::to_wstring(m_DumpCount) + L".txt";
            API::get()->log_info("Printing all objects to %s", File.c_str());

            std::wofstream file(File);
            if (file.is_open()) {
                file << L"Chunked: " << API::FUObjectArray::is_chunked() << L"\n";
                file << L"Inlined: " << API::FUObjectArray::is_inlined() << L"\n";
                file << L"Objects offset: " << API::FUObjectArray::get_objects_offset() << L"\n";
                file << L"Item distance: " << API::FUObjectArray::get_item_distance() << L"\n";
                file << L"Object count: " << API::FUObjectArray::get()->get_object_count() << L"\n";
                file << L"------------\n";

                const auto objects = API::FUObjectArray::get();

                if (objects == nullptr) {
                    file << L"Failed to get FUObjectArray\n";
                    file.close();
                    return;
                }

                std::wstring Categories[2] = { L"Class", L"Function" };
                for (int32_t i = 0; i < objects->get_object_count(); ++i) {
                    const auto object = objects->get_object(i);

                    if (object == nullptr) {
                        continue;
                    }

                    const auto name = object->get_full_name();

                    if (name.empty()) {
                        continue;
                    }

                   

                    file << i << L" " << name << L"\n";
                }

                file.close();
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

			}
		}

			void test_engine(API::UGameEngine * engine) {
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

			void on_pre_engine_tick(API::UGameEngine * engine, float delta) override {
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
			}

    };



    // Actually creates the plugin. Very important that this global is created.
    // The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
    std::unique_ptr<TemplatePlugin> g_plugin{ new TemplatePlugin() };
