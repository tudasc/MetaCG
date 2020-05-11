# ProfileGuided Overhead Estimator

estimates the expected runtime overhead of different measurement methods based on a Cube4 profile

Requires the cube library

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/pgis-test-install -DCXXOPTS_INCLUDE=../deps/src/cxxopts -DJSON_INCLUDE=../deps/src/json/single_include -DCUBE_LIB=$(dirname $(which cube_info))/../lib -DCUBE_INCLUDE=$(dirname $(which cube_info))/../include/cubelib -DEXTRAP_INCLUDE=../deps/src/extrap/extrap-3.0/include -DEXTRAP_LIB=../deps/install/extrap/lib -DSPDLOG_BUILD_SHARED=ON ..
```
