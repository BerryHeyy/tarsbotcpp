# CMakeList.txt : CMake project for tarsbotcpp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("tarsbotcpp")



# Add source to this project's executable.
add_executable (tarsbotcpp "tarsbotcpp.cpp" "tarsbotcpp.h" "program_environment.cpp" "bot_client.h" "botclient.cpp" "program_environment.h" "util.cpp" "util.h")

# TODO: Add tests and install targets if needed.

add_subdirectory(deps/sleepy-discord)
target_link_libraries(tarsbotcpp sleepy-discord)
