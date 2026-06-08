class Textparser < Formula
  desc "High-performance, extensible text parsing library written in C"
  homepage "https://github.com/bokic/textparser"
  head "https://github.com/bokic/textparser.git"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "json-c"
  depends_on "pcre2"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"test.json").write("{\"a\": 1}")
    system "#{bin}/textparser", "test.json", "--mute"
  end
end
