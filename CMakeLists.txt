cmake_minimum_required(VERSION 3.5)
project(test_vsg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vsg REQUIRED)
find_package(vsgXchange REQUIRED)

add_executable(test_vsg src/main.cpp
    include/Application.h
    include/DMD_Reader.h
    include/Mesh.h
    include/stb_image.h

    src/Application.cpp
    src/DMD_Reader.cpp
    src/stb_image.cpp


)

target_include_directories(test_vsg PRIVATE include)
target_link_libraries(test_vsg PRIVATE vsg::vsg vsgXchange::vsgXchange)

include(GNUInstallDirs)
install(TARGETS test_vsg
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
