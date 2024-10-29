cmake_minimum_required(VERSION 3.10)
project(DeribitOrderExecution)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find CURL
find_package(CURL REQUIRED)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
endif()

# Find Boost
find_package(Boost REQUIRED COMPONENTS system)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Find jsoncpp using pkg-config
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

if(JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIRS})
    link_directories(${JSONCPP_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "jsoncpp not found")
endif()

# Include WebSocket++ headers
include_directories(/opt/homebrew/include)  # Adjust if WebSocket++ is installed elsewhere

# Add the executable
add_executable(DeribitOrderExecution main.cpp)

# Link libraries
target_link_libraries(DeribitOrderExecution ${CURL_LIBRARIES} ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} pthread)
