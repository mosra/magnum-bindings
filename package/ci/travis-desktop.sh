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
    -DBUILD_DEPRECATED=$BUILD_DEPRECATED \
    -DWITH_INTERCONNECT=OFF \
    -DWITH_PLUGINMANAGER=OFF \
    -DWITH_TESTSUITE=OFF \
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
    -DWITH_AUDIO=OFF \
    -DWITH_DEBUGTOOLS=OFF \
    -DWITH_GL=OFF \
    -DWITH_MESHTOOLS=OFF \
    -DWITH_PRIMITIVES=OFF \
    -DWITH_SCENEGRAPH=OFF \
    -DWITH_SHADERS=OFF \
    -DWITH_TEXT=OFF \
    -DWITH_TEXTURETOOLS=OFF \
    -DWITH_TRADE=OFF \
    -DWITH_VK=OFF \
    -DBUILD_DEPRECATED=OFF \
    -G Ninja
ninja install
cd ../..

# Build the thing
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/deps \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$HOME/pybind11 \
    -DPYBIND11_PYTHON_VERSION=3.6 \
    -DWITH_PYTHON=ON \
    -G Ninja
ninja

# Verify the setuptools install
cd src/python
python3 setup.py install --root="$TRAVIS_BUILD_DIR/install" --prefix=/usr

# Run tests
cd ../../../src/python/magnum
coverage run -m unittest -v
