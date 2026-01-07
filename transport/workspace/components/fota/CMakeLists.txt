# FOTA CMakeLists.txt
# For ESP-IDF / Standard CMake


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set source files and include directories
set(srcs)
set(include_dirs "include")

# Conditionally add source files
if(CONFIG_FOTA)
    list(APPEND srcs
        "src/fota.cpp"
        "src/command.cpp"
        "src/packet.cpp"
        )
endif()

# Register the component
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}")


target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)