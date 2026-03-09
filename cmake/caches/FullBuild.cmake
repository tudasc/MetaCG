# This is a CMake cache file for a full build
set(CMAKE_BUILD_TYPE
    Release
    CACHE STRING ""
)

# Get external dependencies
set(CUBE_DIR
    "extern/install/cubelib"
    CACHE STRING ""
)
set(EXTRAP_INCLUDE
    "extern/install/extrap/include"
    CACHE STRING ""
)
set(EXTRAP_LIB
    "extern/install/extrap/lib"
    CACHE STRING ""
)

# Enable all features from MetaCG
set(METACG_BUILD_CGCOLLECTOR
    ON
    CACHE BOOL ""
)
set(METACG_BUILD_PGIS
    ON
    CACHE BOOL ""
)
set(METACG_BUILD_GRAPH_TOOLS
    ON
    CACHE BOOL ""
)
