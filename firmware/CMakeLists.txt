cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(FoloToy-AI-Passport)

# ESP-IDF 5.5.3 Security 2 leaves a freed SRP handle in the session on error,
# causing a double free when BLE disconnects. Compile the pinned, minimally
# patched upstream file in the original component until the IDF fix lands.
idf_component_get_property(protocomm_lib protocomm COMPONENT_LIB)
get_target_property(protocomm_sources ${protocomm_lib} SOURCES)
list(FILTER protocomm_sources EXCLUDE REGEX "(^|/)src/security/security2\\.c$")
list(APPEND protocomm_sources "${CMAKE_CURRENT_LIST_DIR}/main/meeting_security2_idf553.c")
set_property(TARGET ${protocomm_lib} PROPERTY SOURCES "${protocomm_sources}")
