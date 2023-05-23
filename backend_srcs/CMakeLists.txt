cmake_minimum_required(VERSION 3.25)
project(ShareShift)

set(CMAKE_CXX_STANDARD 17)

find_package(Poco REQUIRED COMPONENTS Foundation Util Net XML JSON)
add_executable(shareshift_server main.cpp)
target_link_libraries(shareshift_server Poco::Foundation Poco::Util Poco::Net Poco::XML Poco::JSON)
