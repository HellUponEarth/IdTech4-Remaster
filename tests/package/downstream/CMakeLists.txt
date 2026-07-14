cmake_minimum_required(VERSION 3.26)

project(idtech4_downstream_consumer LANGUAGES CXX)

find_package(IdTech4 CONFIG REQUIRED)

add_library(idtech4_downstream_consumer INTERFACE)
target_link_libraries(idtech4_downstream_consumer INTERFACE IdTech4::SDK)
target_compile_features(idtech4_downstream_consumer INTERFACE cxx_std_11)
