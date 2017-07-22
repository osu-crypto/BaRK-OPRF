BINARYDIR := Debug

#Toolchain
CXX := g++
LD := $(CXX)
AR := ar
OBJCOPY := objcopy

#Additional flags
PREPROCESSOR_MACROS := NDEBUG RELEASE
INCLUDE_DIRS := ./bOPRFlib ./thirdparty/linux ./thirdparty/linux/boost/includes ./thirdparty/linux/miracl/miracl_osmt ./thirdparty/linux/mpir ./thirdparty/linux/cryptopp
LIBRARY_DIRS := ./thirdparty/linux/boost/stage/lib ./thirdparty/linux/cryptopp ./thirdparty/linux/miracl/miracl_osmt/source ./thirdparty/linux/mpir/.libs ./thirdparty/linux/ntl/src ./bin/ 
SHARED_LIBRARY_NAMES := pthread rt
STATIC_LIBRARY_NAMES := miracl boost_system boost_filesystem boost_thread mpir cryptopp bOPRF miracl
ADDITIONAL_LINKER_INPUTS := -Wl,--verbose 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CXXFLAGS := -ggdb -ffunction-sections -O3 -Wall -std=c++11 -maes -msse2 -msse4.1 -mpclmul -Wfatal-errors -pthread -Wno-narrowing
LDFLAGS := -Wl,-gc-sections -pthread  
COMMONFLAGS := 

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group
