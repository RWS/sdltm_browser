// version file to be preprocessed by CMake
#ifndef GEN_VERSION_H
#define GEN_VERSION_H
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 60

#define APP_VERSION "1.0.60"

// If it is defined by the compiler, then it is a nightly build, and in the YYYYMMDD format.
#ifndef BUILD_VERSION
    #define BUILD_VERSION "0"
#endif

#endif
