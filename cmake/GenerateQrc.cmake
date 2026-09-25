# GenerateQrc.cmake: auto-generate .qrc files from disk globbing
# Usage: zowi_generate_qrc(OUTPUT_FILE BASE_DIR PREFIX GLOB_PATTERNS...)
# Generates a Qt resource file with <file> entries discovered via glob,
# using alias to preserve the original qrc:/ path structure.
#
# Avoids unnecessary mtime updates: only writes the .qrc file if its
# content has changed. This preserves CMake's incremental build behavior
# (no rcc/recompile/relink on unmodified resource lists).

function(zowi_generate_qrc OUT_FILE BASE_DIR PREFIX)
    # Collect all files matching the glob patterns, re-evaluate on source changes
    file(GLOB_RECURSE FOUND_FILES CONFIGURE_DEPENDS ${ARGN})

    # Sort for reproducible output across reconfigurations
    list(SORT FOUND_FILES)

    # Build the <file> entries
    set(QRC_ENTRIES "")
    foreach(F ${FOUND_FILES})
        # Compute the relative path for the alias (preserves qrc:/ path)
        file(RELATIVE_PATH REL_PATH "${BASE_DIR}" "${F}")
        string(APPEND QRC_ENTRIES "        <file alias=\"${REL_PATH}\">${F}</file>\n")
    endforeach()

    # Construct the final .qrc XML content
    set(NEW_CONTENT "<RCC>\n    <qresource prefix=\"${PREFIX}\">\n${QRC_ENTRIES}    </qresource>\n</RCC>\n")

    # Write only if content changed (preserves mtime if unchanged)
    set(EXISTING_CONTENT "")
    if(EXISTS "${OUT_FILE}")
        file(READ "${OUT_FILE}" EXISTING_CONTENT)
    endif()

    if(NOT EXISTING_CONTENT STREQUAL NEW_CONTENT)
        file(WRITE "${OUT_FILE}" "${NEW_CONTENT}")
    endif()
endfunction()
