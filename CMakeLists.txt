cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(postigWriteChallenge C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)

# Add this to see what's happening during configuration
message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "System Processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Force the compiler to use the include path
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/opt/homebrew/opt/libpq/include")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -L/opt/homebrew/opt/libpq/lib")

# Add executable
add_executable(postigWriteChallenge src/main.c)
include_directories(${PROJECT_SOURCE_DIR}/include)
# Also set it this way to be thorough
target_include_directories(postigWriteChallenge PRIVATE "/opt/homebrew/opt/libpq/include")
target_link_directories(postigWriteChallenge PRIVATE "/opt/homebrew/opt/libpq/lib")
target_link_libraries(postigWriteChallenge PRIVATE pq PRIVATE Threads::Threads)

# Debug output
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
