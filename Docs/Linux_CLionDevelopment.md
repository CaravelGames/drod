# How to develop DROD on Linux with CLion

1. Make sure you got DROD building by following [Linux_Compilation.md](./Linux_Compilation.md).
2. Open the project in CLion.

CLion is very strongly rooted in CMake, but we want to keep compilation in docker. Here are the necessary step:

### Custom Build Target
This is required for the compilation step.

1. Open _Settings &rarr; Custom Build Targets_.
2. Create a new target, call it **"Custom Debug Build"**.
3. Under **Build** click on "&hellip;" and add a new **External Tool**
    1. Name the new tool **"Custom Debug Build (Docker)"**.
    2. In **Program to Run** put `Scripts/Linux/build.sh`
    3. In **Arguments** put `build-drod --mode=custom --debug`
    4. In **Working Directory** put `Scripts/Linux/`
4. Select **"Custom Debug Build (Docker)"** as the build command for the tool.

### Run configuration
This is required to launch the game from CLion.

1. Open _Menu &rarr; Run &rarr; Edit configurations..._.
2. Add a new configuration of type **Custom Build Application** and call it **"Custom Debug"**.
3. For **Target** select **Custom Debug Build**.
4. In **Executable** put `Master/Linux/builds/custom.debug.x86_64/drod` (the file won't exist unless you have already build the game once).
5. In **Working Directory** put `Master/Linux/builds/caravel.debug.x86_64`


### Code completion
For code completion to work you need to create `CMakeLists.txt` in the root directory of the project:

```cmake
cmake_minimum_required(VERSION 3.20)
project(DrodCPP)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 11)

# Point to your compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dummy target so CLion is happy
file(GLOB_RECURSE ALL_SOURCES "*.cpp" "*.h")
add_executable(dummy_for_clion ${ALL_SOURCES})
target_compile_options(dummy_for_clion PRIVATE -x c++)
target_compile_options(dummy_for_clion PRIVATE -std=c++11)
target_include_directories(dummy_for_clion PRIVATE
        ./
        ./FrontEndLib
        ./BackEndLib
        ./DROD
        ./DRODLib
        ./DRODLibTests
        ./CaravelNet
        ./metakit/include
)
```


### Breakpoints and debugging
In order for path mapping to work correctly in CLion create file `~/.gdbinit` (in your home directory) with the following contents:

```text
set substitute-path /drod /home/example/DROD
```

Replace `/home/example/DROD` with the absolute path to the directory where DROD is checked out, no trailing slash.

### Finally
You should now be able to build DROD and debug it in CLion using its native build capabilities.