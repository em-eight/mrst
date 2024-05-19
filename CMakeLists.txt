cmake_minimum_required (VERSION 3.5)
project (mrst)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# Detect if we're using MinGW
if(MINGW)
    # Add the static libgcc and libstdc++ flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ --static")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ --static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ --static")
endif()

set(RSND_SRC ${sources}
    src/rsnd/soundCommon.cpp
    src/rsnd/SoundArchive.cpp
    src/rsnd/SoundWaveArchive.cpp
    src/rsnd/SoundWave.cpp
    src/rsnd/SoundBank.cpp
    src/rsnd/SoundStream.cpp
    src/rsnd/SoundSequence.cpp
    src/rsnd/SoundWsd.cpp

    src/common/util.cpp
    src/common/fileUtil.cpp
    src/tools/extract.cpp
    src/tools/decode.cpp
    src/tools/list.cpp
    src/tools/common.cpp

    # VGMTrans
    external/vgmtrans/ScaleConversion.cpp
    external/vgmtrans/RiffFile.cpp
    external/vgmtrans/SF2File.cpp
    external/vgmtrans/MidiFile.cpp
)
add_library(rsnd ${RSND_SRC})
target_include_directories(rsnd PUBLIC include external)


add_executable(mrst src/mrst.cpp)
target_include_directories(mrst PUBLIC include)
target_link_libraries(mrst rsnd)

install(TARGETS rsnd EXPORT export_rsnd
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# executables
install(TARGETS mrst DESTINATION bin)

# headers
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})