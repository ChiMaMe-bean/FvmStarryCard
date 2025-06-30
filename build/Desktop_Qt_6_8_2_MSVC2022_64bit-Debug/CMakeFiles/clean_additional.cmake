# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\stellarforge_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\stellarforge_autogen.dir\\ParseCache.txt"
  "stellarforge_autogen"
  )
endif()
