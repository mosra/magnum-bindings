class MagnumBindings < Formula
  desc "`Bindings for the Magnum C++11/C++14 graphics engine"
  homepage "https://magnum.graphics"
  url "https://github.com/mosra/magnum-bindings/archive/v2020.06.tar.gz"
  # wget https://github.com/mosra/magnum-bindings/archive/v2020.06.tar.gz -O - | sha256sum
  sha256 "959c703e6409ba0c2cd6c0da3a2b6190f6fac837ff69f64cbdc372e11359e7d8"
  head "git://github.com/mosra/magnum-bindings.git"

  depends_on "cmake"
  depends_on "python"
  depends_on "magnum"
  depends_on "pybind11" => :build

  def install
    system "mkdir build"
    cd "build" do
      system "cmake",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=#{prefix}",
        "-DWITH_PYTHON=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
      cd "src/python" do
        system "python3", *Language::Python.setup_install_args(prefix)
      end
    end
  end
end
