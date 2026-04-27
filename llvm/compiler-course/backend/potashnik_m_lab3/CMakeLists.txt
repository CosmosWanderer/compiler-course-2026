get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME "${CURRENT_DIR_NAME}_BACKEND")

file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)

add_llvm_pass_plugin(${TARGET_NAME}
    ${SOURCES}
    DEPENDS
    intrinsics_gen
    X86
    BUILDTREE_ONLY
)

target_include_directories(${TARGET_NAME} PUBLIC ${PATH_TO_X86})
set(LLVM_TEST_DEPENDS ${TARGET_NAME} ${LLVM_TEST_DEPENDS} PARENT_SCOPE)
