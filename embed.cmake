# Writes every file under RESOURCES into one C source as a byte array, with a header naming each as an Embed.
# resources/icon/folder_open.svg becomes ICON_FOLDER_OPEN_SVG.
# Each array ends in a zero not counted in its size, so text can be read as a C string.
# Run as a script: cmake -DRESOURCES=<directory> -DOUTPUT=<directory> -P embed.cmake
file(GLOB_RECURSE FILES RELATIVE ${RESOURCES} ${RESOURCES}/*)
list(FILTER FILES EXCLUDE REGEX "LICENSE")
list(SORT FILES)

set(HEADER "#pragma once\n\n#ifdef __cplusplus\nextern \"C\"\n{\n#endif\n\nstruct Embed\n{\n    const unsigned char* data;\n    unsigned int size;\n};\n\n")
set(SOURCE "#include \"embed.h\"\n\n")
foreach(FILE ${FILES})
    string(MAKE_C_IDENTIFIER ${FILE} NAME)
    string(TOUPPER ${NAME} NAME)
    file(READ ${RESOURCES}/${FILE} BYTES HEX)
    string(REGEX REPLACE "(..)" "0x\\1," BYTES "${BYTES}")
    string(APPEND HEADER "extern const struct Embed ${NAME};\n")
    string(APPEND SOURCE "static const unsigned char ${NAME}_DATA[] = {${BYTES}0};\n")
    string(APPEND SOURCE "const struct Embed ${NAME} = {${NAME}_DATA, sizeof(${NAME}_DATA) - 1};\n")
endforeach()
string(APPEND HEADER "\n#ifdef __cplusplus\n}\n#endif\n")

# The header is only replaced when it changes, so editing a resource doesn't rebuild what includes it.
file(WRITE ${OUTPUT}/embed.h.new "${HEADER}")
file(COPY_FILE ${OUTPUT}/embed.h.new ${OUTPUT}/embed.h ONLY_IF_DIFFERENT)
file(WRITE ${OUTPUT}/embed.c "${SOURCE}")
