# Rocket-Engine
## Introduction

**Rocket-Engine** is a 2D Game engine based on ECS architecture and **C++20** `modules`.  
It is designed to be cross-platform, but currently it only supports **Windows(x86-64)**.

The engine is made up of three projects. **Rocket**(dynamic lib) is the engine kernel; **RocketGlue**(static lib) is the native-scripting(registering) support; **RocketLauncher**(executable) is the engine editor.

At first, this is just a toy engine, step by step created, following the **Hazel 2D** tutorial by **TheCherno**. As it develops, the scale of the project has become big enough to actually own some productivity. This engine is still on development and will soon become usable for cross-platform game making.

---

Great thanks to **TheCherno** and his tutorial.

Youtube account: https://www.youtube.com/@TheCherno  
Hazel 2D: https://github.com/TheCherno/Hazel
 
## How to build?

The project uses **CMake**(version greater than 3.31) and **vcpkg** as building support. Make sure the are correctly installed and added to the environment.  
On **Windows**, `MSVC` is the first(now the only) choice as the C++ compiler. Maybe `clang(llvm)` can work, but untested.  
With **Visual Studio 2026**, you don't need a single line in console to build.

Clone the whole code base; Open `CMakesLists.txt` with **Visual Studio 2026**, after CMake configuration, press **Build -> Build All**. Then you should see the compiled files under the folder `bin`.

## How to script?

Now, the engine only support native-scripting(which means also use **C++20** `modules` to script).
In the future, a scripting languange similar to `GDScript` and `Python` will be implemented and introduced.

After successfully building the engine, press **Build -> Install RocketEngine**; Then you should see a folder named ``RocketSDK`` under the root dir; Add it to the **Environment Path**.

Run `RocketEngine.exe`, press **File -> New Project**. You don't need to close the editor. Open the generated `CMakeLists.txt` with **Visual Studio 2026**; You can write codes in it and build the script project. Then press the **Reload Script** button on the toolbar of the editor, and your scripts are good to use.

## Third-Party Reliance

This engine relies on multiple C/C++ Third-Party libraries.

**glad**(OpenGL support): https://github.com/Dav1dde/glad  
**glfw**(Multi-platform window support): https://github.com/glfw/glfw  
**glm**(3D Graphics math support): https://github.com/icaven/glm  
**Box2D**(2D Physics engine): https://github.com/erincatto/box2d  
**EnTT**(ECS support): https://github.com/skypjack/entt  
**yaml-cpp**(Serialization support): https://github.com/jbeder/yaml-cpp  
**stb**(Font & Image loading support): https://github.com/nothings/stb  
**shaderc**(glsl support): https://github.com/google/shaderc  
**SPIRV-Cross**(glsl support): https://github.com/KhronosGroup/SPIRV-Cross

**ImGui**(Editor GUI support): https://github.com/ocornut/imgui  
**ImGuizmo**(Editor gizmo support): https://github.com/CedricGuillemet/ImGuizmo
