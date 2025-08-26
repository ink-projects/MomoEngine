cmake_minimum_required(VERSION 3.14...3.31)
project(
    momoengine # Name this whatever you want
    LANGUAGES CXX C
)

add_executable( helloworld demo/helloworld.cpp )
set_target_properties( helloworld PROPERTIES CXX_STANDARD 20 )
add_custom_target( run_helloworld helloworld USES_TERMINAL )