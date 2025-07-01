# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\starrycard_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\starrycard_autogen.dir\\ParseCache.txt"
  "starrycard_autogen"
  )
endif()
