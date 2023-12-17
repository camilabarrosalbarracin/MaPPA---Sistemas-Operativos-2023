# Libraries
LIBS=Shared commons readline

# Custom libraries' paths
SHARED_LIBPATHS=
STATIC_LIBPATHS=../Shared

# Compiler flags
CDEBUG=-g -Wall -DDEBUG -fcommon -fdiagnostics-color=always
CRELEASE=-O3 -Wall -DNDEBUG -fcommon

# Arguments when executing with start, memcheck or helgrind
ARGS=

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes
HELGRIND_FLAGS=--log-file="helgrind.log"

# Source files (*.c) to be excluded from tests compilation
TEST_EXCLUDE=src/main.c
