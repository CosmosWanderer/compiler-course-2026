get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME "${CURRENT_DIR_NAME}_ClangAST")

file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)
add_llvm_library(${TARGET_NAME} MODULE ${SOURCES} PLUGIN_TOOL clang)

if(WIN32 OR CYGWIN)
    set(LLVM_LINK_COMPONENTS Support)
    clang_target_link_libraries(${TARGET_NAME} PRIVATE
        clangAST
        clangBasic
        clangFrontend
    )
endif()

set(CLANG_TEST_DEPS "${TARGET_NAME}" ${CLANG_TEST_DEPS} PARENT_SCOPE)
