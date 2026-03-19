
SET(SUBDIRS
    utils
    mesh
    solver
    io
)

FOREACH(SUB ${SUBDIRS})
    ADD_SUBDIRECTORY(${SUB})
ENDFOREACH()

ADD_EXECUTABLE(tinytcad main.cpp)

TARGET_INCLUDE_DIRECTORIES(tinytcad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/solver
    ${CMAKE_CURRENT_SOURCE_DIR}/io
)

TARGET_LINK_LIBRARIES(tinytcad PRIVATE
    utils
    mesh
    solver
    io
)
