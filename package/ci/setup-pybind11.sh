set -e

wget --no-clobber https://github.com/pybind/pybind11/archive/v2.2.4.tar.gz && tar -xzf v2.2.4.tar.gz

cd pybind11-2.2.4

# Patch the CMake macro to add include paths as SYSTEM because otherwise GCC
# 4.8 gets very loud. This is already done in
# https://github.com/pybind/pybind11/pull/1416 but it's opt-in and not part of
# any release yet. I don't want to force users to use pybind11 git, so apply
# a patch manually.
patch -p1 < $TRAVIS_BUILD_DIR/package/ci/pybind11-system-includes.patch

mkdir -p build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=$HOME/pybind11 \
    -DPYBIND11_PYTHON_VERSION=3.6 \
    -DPYBIND11_TEST=OFF \
    -G Ninja
ninja install
