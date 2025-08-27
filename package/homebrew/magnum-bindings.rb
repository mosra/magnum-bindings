class MagnumBindings < Formula
  desc "`Bindings for the Magnum C++11 graphics engine"
  homepage "https://magnum.graphics"
  # git describe origin/master, except the `v` prefix
  version "2020.06-421-g439945c"
  # Clone instead of getting an archive to have tags for version.h generation
  url "https://github.com/mosra/magnum-bindings.git", revision: "439945c"
  head "https://github.com/mosra/magnum-bindings.git"

  depends_on "cmake" => :build
  depends_on "python"
  depends_on "python-setuptools" => :build
  depends_on "magnum"
  depends_on "pybind11" => :build

  def install
    system "mkdir build"
    cd "build" do
      system "cmake",
        *std_cmake_args,
        # Without this, ARM builds will try to look for dependencies in
        # /usr/local/lib and /usr/lib (which are the default locations) instead
        # of /opt/homebrew/lib which is dedicated for ARM binaries. Please
        # complain to Homebrew about this insane non-obvious filesystem layout.
        "-DCMAKE_INSTALL_NAME_DIR:STRING=#{lib}",
        "-DMAGNUM_WITH_PYTHON=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
      cd "src/python" do
        system "python3", *Language::Python.setup_install_args(prefix)
      end
    end
  end
end
