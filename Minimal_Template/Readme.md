UEVR Template Projects by lobotomyx

Much of the code here was lifted from praydog's examples but some has been modified
The main appeal here is the project setups themselves which have been redone using macros to setup paths
No messing around with cmake or cmkr. Even git isn't strictly necessary here,
And in fact there's no point in cloning unless you want to contribute a change

You can save your project anywhere you want, it literally doesn't matter
If you want to add dependencies, instead of messing around with build systems, simply clone or copy the project into your project directory
Right click the project in VS, go to VC++ Directories, edit the include directories by clicking the dropdown on the far right, add a new line, and enter
```$(ProjectDir)/[dependency name]```
Include paths are not recursive. Suppose you add MinHook to your project and do as told above. Now to access MinHook from a class you need to type 
```#include "MinHook/Include/MinHook.h"```
If you want to be able to type ```#include <MinHook>``` you would add ```$(ProjectDir)/MinHook/Include``` as well


The Minimalist template has no requirements aside from Windows SDK and Visual Studio
The GUI template project includes Dear ImGui 
ImGui implementation taken from praydog's UEVR example plugin
ImGui has been included as a static package to ensure compilation and avoid issues with git ssh downloads
ImGui package has been stripped of unnecessary backends and example projects for file size
If you want to do anything more complex with ImGui its recommended to clone the repository separately and build the examples
To actually learn about its usage I highly recommend visiting the interactive manual https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html

Please note that while the minimalist version does not include imgui, UEVR does expose its own ImGui implementation to the Lua API
I have provided functions in the template to send and received data to lua scripts. If you are new to ImGui this is a better way to use it
Lua's design as a configuration language before its scripting capabilities makes it quicker to use especially for non-programmers

Lua scripts can also be used for the majority of UEVR functions and you may be better off using that instead of C++
I personally find Lua lacking as a language with python being my weapon of choice for scripting
The UEVR API is largely the same in lua or C++ so the main reasons you should consider C++ are if you simply prefer a "real" programming language for syntax,
Or if you want to do any kind of FFI, filesystem tasks outside the Appdata/UnrealVRMod sandbox, or direct memory access

