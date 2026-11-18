
aux_source_directories(SOURCES "Source Files"                    .)
aux_source_directories(SOURCES "Source Files\\hexdom"            hexdom)
aux_source_directories(SOURCES "Source Files\\optimal_transport" optimal_transport)

# too many warnings in hexdom when compiled with Apple clang
if(
   "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
   "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang"
)
   add_definitions(-Wno-extra-semi-stmt) # duplicated semicolons
endif()

include_directories(${PROJECT_BINARY_DIR}/src/lib)

add_library(exploragram ${SOURCES})

set_target_properties(exploragram PROPERTIES
VERSION ${VORPALINE_VERSION}
SOVERSION ${VORPALINE_VERSION_MAJOR}
FOLDER "GEOGRAM")

if(WIN32)
target_link_libraries(exploragram psapi)
endif()

if(GEOGRAM_WITH_VORPALINE)
target_link_libraries(exploragram vorpalib)
endif()

target_link_libraries(exploragram geogram)

set_target_properties(
exploragram PROPERTIES
FOLDER "GEOGRAM")
