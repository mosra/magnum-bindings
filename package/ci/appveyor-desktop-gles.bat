if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2022" call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2019" call "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" call "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 || exit /b
set PATH=%APPVEYOR_BUILD_FOLDER%\deps\bin;%PATH%

rem Build pybind11. Downloaded in the appveyor.yml script.
cd pybind11-%PYBIND% || exit /b
mkdir -p build && cd build || exit /b
cmake .. ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DPYBIND11_PYTHON_VERSION=3.%PYTHON% ^
    -DPYBIND11_TEST=OFF ^
    -G Ninja || exit /b
ninja install || exit /b
cd .. && cd ..

rem Build Corrade
git clone --depth 1 https://github.com/mosra/corrade.git || exit /b
cd corrade || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DCORRADE_BUILD_DEPRECATED=OFF ^
    -DCORRADE_BUILD_STATIC=%BUILD_STATIC% ^
    -DCORRADE_WITH_INTERCONNECT=OFF ^
    -DCORRADE_WITH_PLUGINMANAGER=ON ^
    -DCORRADE_WITH_TESTSUITE=ON ^
    -DCORRADE_UTILITY_USE_ANSI_COLORS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum
git clone --depth 1 https://github.com/mosra/magnum.git || exit /b
cd magnum || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_BUILD_DEPRECATED=OFF ^
    -DMAGNUM_BUILD_STATIC=%BUILD_STATIC% ^
    -DMAGNUM_TARGET_GLES=ON ^
    -DMAGNUM_TARGET_GLES2=%TARGET_GLES2% ^
    -DMAGNUM_TARGET_EGL=OFF ^
    -DMAGNUM_WITH_AUDIO=OFF ^
    -DMAGNUM_WITH_DEBUGTOOLS=OFF ^
    -DMAGNUM_WITH_MATERIALTOOLS=OFF ^
    -DMAGNUM_WITH_GL=ON ^
    -DMAGNUM_WITH_MESHTOOLS=ON ^
    -DMAGNUM_WITH_PRIMITIVES=ON ^
    -DMAGNUM_WITH_SCENEGRAPH=ON ^
    -DMAGNUM_WITH_SCENETOOLS=ON ^
    -DMAGNUM_WITH_SHADERS=ON ^
    -DMAGNUM_WITH_SHADERTOOLS=OFF ^
    -DMAGNUM_WITH_TEXT=ON ^
    -DMAGNUM_WITH_TEXTURETOOLS=ON ^
    -DMAGNUM_WITH_TRADE=ON ^
    -DMAGNUM_WITH_VK=OFF ^
    -DMAGNUM_WITH_SDL2APPLICATION=OFF ^
    -DMAGNUM_WITH_GLFWAPPLICATION=OFF ^
    -DMAGNUM_WITH_WINDOWLESSWGLAPPLICATION=ON ^
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON ^
    -DMAGNUM_WITH_ANYSCENECONVERTER=ON ^
    -DMAGNUM_WITH_ANYSCENEIMPORTER=ON ^
    -DMAGNUM_WITH_TGAIMPORTER=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git || exit /b
cd magnum-plugins || exit /b
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DMAGNUM_BUILD_STATIC=%BUILD_STATIC% ^
    -DMAGNUM_WITH_BCDECIMAGECONVERTER=ON ^
    -DMAGNUM_WITH_DDSIMPORTER=ON ^
    -DMAGNUM_WITH_ETCDECIMAGECONVERTER=ON ^
    -DMAGNUM_WITH_GLTFIMPORTER=ON ^
    -DMAGNUM_WITH_GLTFSCENECONVERTER=ON ^
    -DMAGNUM_WITH_KTXIMAGECONVERTER=ON ^
    -DMAGNUM_WITH_MESHOPTIMIZERSCENECONVERTER=ON ^
    -DMAGNUM_WITH_PRIMITIVEIMPORTER=ON ^
    -DMAGNUM_WITH_STANFORDSCENECONVERTER=ON ^
    -DMAGNUM_WITH_STBIMAGECONVERTER=ON ^
    -DMAGNUM_WITH_STBIMAGEIMPORTER=ON ^
    -DMAGNUM_WITH_STBRESIZEIMAGECONVERTER=ON ^
    -DMAGNUM_WITH_STBTRUETYPEFONT=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b
cd .. && cd ..

rem Build. BUILD_GL_TESTS is enabled just to be sure, it should not be needed
rem by any plugin.
mkdir build && cd build || exit /b
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%APPVEYOR_BUILD_FOLDER%/deps ^
    -DPYBIND11_PYTHON_VERSION=3.%PYTHON% ^
    -DMAGNUM_WITH_PYTHON=ON ^
    -DMAGNUM_BUILD_TESTS=ON ^
    -G Ninja || exit /b
cmake --build . || exit /b
cmake --build . --target install || exit /b

rem Test
set CORRADE_TEST_COLOR=ON
rem On Windows, if an assertion or other issue happens, A DIALOG WINDOWS POPS
rem UP FROM THE CONSOLE. And then, for fucks sake, IT WAITS ENDLESSLY FOR YOU
rem TO CLOSE IT!! Such behavior is utterly stupid in a non-interactive setting
rem such as on this very CI, so I'm setting a timeout to 60 seconds to avoid
rem the CI job being stuck for an hour if an assertion happens. CTest's default rem timeouts is somehow 10M seconds, which is as useful as nothing at all.
ctest -V -E GLTest --timeout 60 || exit /b

rem Verify the setuptools install
cd src/python || exit /b
python setup.py install --root="%APPVEYOR_BUILD_FOLDER%/install" || exit /b

rem Run python tests & gather coverage
cd ../../../src/python/corrade || exit /b
coverage run -m unittest -v || exit /b
cp .coverage ../.coverage.corrade || exit /b

cd ../magnum || exit /b
set MAGNUM_SKIP_GL_TESTS=ON
coverage run -m unittest -v || exit /b
cp .coverage ../.coverage.magnum || exit /b

rem Test docstring validity
cd ../../../doc/python || exit /b
set PYTHONPATH="%APPVEYOR_BUILD_FOLDER%/build/src/python"
rem We (deliberately) don't have numpy installed, so this would fail
rem TODO: any idea how to fix this? doctest has SKIPs but not conditional ones
rem python -m doctest -v *.rst || exit /b

rem Upload coverage
cd ../../src/python || exit /b
coverage combine || exit /b
rem TODO: Currently disabled because I can't seem to convince it to relocate
rem the paths via codecov.yml: https://github.com/mosra/magnum-bindings/pull/3
rem codecov -X gcov || exit /b
