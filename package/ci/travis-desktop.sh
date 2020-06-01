#!/bin/bash
set -ev

# Corrade
git clone --depth 1 git://github.com/mosra/corrade.git
cd corrade
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_DEPRECATED=OFF \
    -DBUILD_STATIC=$BUILD_STATIC \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=ON \
    -DWITH_TESTSUITE=ON \
    -G Ninja
ninja install
cd ../..

# Magnum
git clone --depth 1 git://github.com/mosra/magnum.git
cd magnum
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_DEPRECATED=OFF \
    -DBUILD_STATIC=$BUILD_STATIC $STATIC_PLUGIN_PATH \
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_GL=ON \
    -DWITH_MESHTOOLS=ON \
    -DWITH_PRIMITIVES=ON \
    -DWITH_SCENEGRAPH=ON \
    -DWITH_SHADERS=ON \
    -DWITH_TEXT=OFF \
    -DWITH_TEXTURETOOLS=OFF \
    -DWITH_TRADE=ON \
    -DWITH_VK=OFF \
    -DWITH_GLFWAPPLICATION=ON \
    -DWITH_SDL2APPLICATION=ON \
    -DWITH_WINDOWLESS${PLATFORM_GL_API}APPLICATION=ON \
    -DWITH_ANYIMAGEIMPORTER=ON \
    -G Ninja
ninja install
cd ../..

# Magnum Plugins
git clone --depth 1 git://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_INSTALL_RPATH=$HOME/deps/lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC=$BUILD_STATIC \
    -DWITH_DDSIMPORTER=ON \
    -DWITH_STBIMAGEIMPORTER=ON \
    -DWITH_TINYGLTFIMPORTER=ON \
    -G Ninja
ninja install
cd ../..

# Build the thing
mkdir build && cd build
cmake .. \
    -DCMAKE_CXX_FLAGS="--coverage" \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=$HOME/pybind11 \
    -DPYBIND11_PYTHON_VERSION=3.6 \
    -DWITH_PYTHON=ON \
    -DBUILD_TESTS=ON \
    -G Ninja
ninja

CORRADE_TEST_COLOR=ON ctest -V

# Verify the setuptools install
cd src/python
python3 setup.py install --root="$TRAVIS_BUILD_DIR/install" --prefix=/usr

# Run tests & gather coverage
cd ../../../src/python/corrade
coverage run -m unittest -v
cp .coverage ../.coverage.corrade

cd ../magnum
MAGNUM_SKIP_GL_TESTS=ON coverage run -m unittest -v
cp .coverage ../.coverage.magnum

# Test docstring validity
cd ../../../doc/python
PYTHONPATH="$TRAVIS_BUILD_DIR/build/src/python" python3 -m doctest -v *.rst
