cmake_minimum_required(VERSION 3.0.2)

# メジャーバージョンの設定。複雑さと使いやすさのバランスをとって手動更新としている。
set(MAJOR_VERSION 0)
set(MAJOR_VERSION_COMMIT_OFFSET 0)

set(MINOR_VERSION 0)
set(MINOR_VERSION_COMMIT_OFFSET 0)

execute_process(
  COMMAND git log --oneline COMMAND wc --lines
  OUTPUT_VARIABLE TOTAL_COMMITS
)
math(EXPR SUBMINOR_VERSION
  "${TOTAL_COMMITS} - ${MAJOR_VERSION_COMMIT_OFFSET} - ${MINOR_VERSION_COMMIT_OFFSET}"
)

project(openpso VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${SUBMINOR_VERSION} LANGUAGES CXX)

set(${PROJECT_NAME}_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++17")

find_package(Boost REQUIRED)

include_directories(
  ${${PROJECT_NAME}_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)

FILE(GLOB ${PROJECT_NAME}_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
if (${PROJECT_NAME}_SOURCES) # 実のところこの条件は必要ない
  foreach(EACH IN LISTS ${PROJECT_NAME}_SOURCES)
    string(REGEX REPLACE "^/(.*/)*(.*).cpp$" "\\2" TARGET ${EACH})
    add_executable(${TARGET} ${EACH})
    target_link_libraries(${TARGET} ${Boost_LIBRARIES})
  endforeach()
endif()

