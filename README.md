# What is Gkm-Local?

Gkm-Local is a game prototype.

# Dependencies and build instructions

1. BGFX library

You should clone bgfx git repository and its dependencies.
In some folder with writting permissions please run:
```
git clone https://github.com/bkaradzic/bgfx
git clone https://github.com/bkaradzic/bimg
git clone https://github.com/bkaradzic/bx
cd bgfx
..\bx\tools\bin\windows\genie --with-tools --with-examples vs2019
```

Open the generated bgfx\\.build\projects\bgfx.sln in Visual Studio IDE and switch build type to 'Release', switch architecture to 'x64'.
Probably you need to retarget the solution. Right mouse click on solution and select 'Retarget solution' to the latest available Windows SDK.
Build it (F7 key usually). When switch build type to 'Debug' and build again.

2. Microsoft VCPKG

You should clone vcpkg git repository.
In some folder with writting permissions please run:
```
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
vcpkg install libjpeg-turbo --triplet x64-windows-static
```

3. During CMake configure step specify variables BGFX_ROOT and VCPKG_ROOT
to the corresponding directories of the cloned git repositories.

# Data structure and threads

[Main thread] -> (keyboard state) -> [Game logic thread]

[Game logic thread] -> (player coordinates) -> [Render thread]

[Game logic thread] -> (block operation requests queue) -> [Block operation thread]

[Block operation thread] -> (blocks hierarchy, world data) -> [Render thread for drawing]

[World thread]           ->? (blocks hierarchy, world data)

[Game logic thread]      -> (block tessellation requests queue) -> [Tessellation threads]

[Block operation thread] -> (block tessellation requests queue)

[World thread]           ->? (block tessellation requests queue)

[Tessellation threads] -> (block vertex buffers) -> [Render thread for drawing]

# TODO List

* Block selection, brush size selection, block operations
* Improve tessellation of blocks
* Background world unloading, generation and loading
* Effective cell space 3D partitioning
