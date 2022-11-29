from conans import ConanFile, CMake, tools

Options = [
    # Cafe
    ("CAFE_INCLUDE_TESTS", [True, False], False),

    # Cafe.Encoding
    ("CAFE_ENCODING_INCLUDE_ENCODING_LIST", "ANY", "UTF-8,UTF-16,UTF-32,GB2312"),
    ("CAFE_ENCODING_INCLUDE_UNICODE_DATA", [True, False], True),
    ("CAFE_ENCODING_INCLUDE_RUNTIME_ENCODING", [True, False], True),
]


class CafeEncodingConan(ConanFile):
    name = "Cafe.Encoding"
    version = "0.1"
    license = "MIT"
    author = "akemimadoka <chino@hotococoa.moe>"
    url = "https://github.com/akemimadoka/Cafe.Encoding"
    description = "A general purpose C++ library"
    topics = "C++"
    settings = "os", "compiler", "build_type", "arch"
    options = {opt[0]: opt[1] for opt in Options}
    default_options = {opt[0]: opt[2] for opt in Options}

    requires = "Cafe.Core/0.1"
    python_requires = "CafeCommon/0.1"

    generators = "cmake"

    exports_sources = "CMakeLists.txt", "CafeCommon*", "Base*", "GB2312*", "RuntimeEncoding*", "UnicodeData*", "UTF-8*", "UTF-16*", "UTF-32*", "Test*"

    def requirements(self):
        if self.options.CAFE_INCLUDE_TESTS:
            self.requires("catch2/3.2.0", private=True)

    def configure_cmake(self):
        cmake = CMake(self)
        for opt in Options:
            cmake.definitions[opt[0]] = str(getattr(self.options, opt[0])).replace(",", ";")
        cmake.configure()
        return cmake

    def build(self):
        with tools.vcvars(self.settings, filter_known_paths=False) if self.settings.compiler == 'Visual Studio' else tools.no_op():
            cmake = self.configure_cmake()
            cmake.build()

    def package(self):
        with tools.vcvars(self.settings, filter_known_paths=False) if self.settings.compiler == 'Visual Studio' else tools.no_op():
            cmake = self.configure_cmake()
            cmake.install()

    def package_info(self):
        self.python_requires["CafeCommon"].module.addCafeSharedCompileOptions(self)
        self.cpp_info.libs = [
            "Cafe.Encoding.RuntimeEncoding", "Cafe.Encoding.UnicodeData"]
