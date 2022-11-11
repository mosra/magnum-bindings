#!/bin/bash
set -ev

# Corrade
git clone --depth 1 https://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORRADE_BUILD_DEPRECATED=OFF \
    -DCORRADE_WITH_INTERCONNECT=OFF \
    -DCORRADE_WITH_PLUGINMANAGER=ON \
    -DCORRADE_WITH_TESTSUITE=ON \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 https://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$HOME/swiftshader;$HOME/pybind11" \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DMAGNUM_BUILD_DEPRECATED=OFF \
    -DMAGNUM_TARGET_GLES=ON \
    -DMAGNUM_TARGET_GLES2=$TARGET_GLES2 \
    -DMAGNUM_WITH_AUDIO=OFF \
    -DMAGNUM_WITH_DEBUGTOOLS=OFF \
    -DMAGNUM_WITH_GL=ON \
    -DMAGNUM_WITH_MATERIALTOOLS=OFF \
    -DMAGNUM_WITH_MESHTOOLS=ON \
    -DMAGNUM_WITH_PRIMITIVES=ON \
    -DMAGNUM_WITH_SCENEGRAPH=ON \
    -DMAGNUM_WITH_SCENETOOLS=OFF \
    -DMAGNUM_WITH_SHADERS=ON \
    -DMAGNUM_WITH_SHADERTOOLS=OFF \
    -DMAGNUM_WITH_TEXT=ON \
    -DMAGNUM_WITH_TEXTURETOOLS=OFF \
    -DMAGNUM_WITH_TRADE=ON \
    -DMAGNUM_WITH_VK=OFF \
    -DMAGNUM_WITH_WINDOWLESSEGLAPPLICATION=ON \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON \
    -DMAGNUM_WITH_ANYSCENECONVERTER=ON \
    -G Ninja
ninja install
cd ../..

# Magnum Plugins
git clone --depth 1 https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    `# StanfordSceneConverter uses MeshTools which rely on GL which looks` \
    `# for OpenGLES. And the RPATH entry needs to be there as well,` \
    `# otherwise it won't load.` \
    -DCMAKE_PREFIX_PATH=$HOME/swiftshader \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DMAGNUM_BUILD_STATIC=$BUILD_STATIC \
    -DMAGNUM_WITH_DDSIMPORTER=ON \
    -DMAGNUM_WITH_GLTFIMPORTER=ON \
    -DMAGNUM_WITH_STANFORDSCENECONVERTER=ON \
    -DMAGNUM_WITH_STBIMAGECONVERTER=ON \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=ON \
    -DMAGNUM_WITH_STBTRUETYPEFONT=ON \
    -G Ninja
ninja install
cd ../..

# Build the thing
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="--coverage" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$HOME/swiftshader;$HOME/pybind11" \
    -DCMAKE_INSTALL_RPATH="$HOME/deps/lib;$HOME/swiftshader/lib" \
    -DPYBIND11_PYTHON_VERSION=3.6 \
    -DMAGNUM_WITH_PYTHON=ON \
    -DMAGNUM_BUILD_TESTS=ON \
    -G Ninja
ninja $NINJA_JOBS

CORRADE_TEST_COLOR=ON ctest -V

# Verify the setuptools install. Not using $CIRCLE_WORKING_DIRECTORY because
# it's ~ and that's not correctly expanded always.
cd src/python
python3 setup.py install --root="$(pwd)/../../install" --prefix=/usr

# Run tests & gather coverage
cd ../../../src/python/corrade
coverage run -m unittest -v
cp .coverage ../.coverage.corrade

cd ../magnum
coverage run -m unittest -v
cp .coverage ../.coverage.magnum

# Test docstring validity
cd ../../../doc/python
# I would use $CIRCLE_WORKING_DIRECTORY, but that's ~ and that's not expanded
# here for some reason
PYTHONPATH="$(pwd)/../../build/src/python" python3 -m doctest -v *.rst
