get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME "${CURRENT_DIR_NAME}_LLVM_IR")

if (NOT WIN32 AND NOT CYGWIN)
    file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)
    add_llvm_pass_plugin(${TARGET_NAME} ${SOURCES}
        DEPENDS
        intrinsics_gen
        BUILDTREE_ONLY
    )
    set(LLVM_TEST_DEPENDS ${TARGET_NAME} ${LLVM_TEST_DEPENDS} PARENT_SCOPE)
endif()
