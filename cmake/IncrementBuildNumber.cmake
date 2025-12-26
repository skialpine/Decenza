# Read current build number
file(READ "${BUILD_NUMBER_FILE}" BUILD_NUMBER)
string(STRIP "${BUILD_NUMBER}" BUILD_NUMBER)

# Increment
math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")

# Write back
file(WRITE "${BUILD_NUMBER_FILE}" "${BUILD_NUMBER}\n")

# Generate header
file(WRITE "${OUTPUT_HEADER}"
"#pragma once
// Auto-generated - do not edit
#define BUILD_NUMBER ${BUILD_NUMBER}
#define BUILD_NUMBER_STRING \"${BUILD_NUMBER}\"
")

message(STATUS "Build number: ${BUILD_NUMBER}")
