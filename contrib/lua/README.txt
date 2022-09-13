Building Lua
============

1. cd lua-5.1.3
2. Install libreadline5-dev using Synaptic if its not installed.
3. Refer to the README and INSTALL files.
4. make (to find out supported targets)
5. make linux (to build on linux)
6. make test
7. sudo make install

Building luasocket module
=========================

1. cd luasocket-2.0.2
2. make
3. sudo make install

#@todo Note: this is roadmapped to be made part of the EHS source build
#@todo review the icensing: LGPL?


Building lfs (lua file system) module
=========================
1. rename appropriate Makefile (windows or linux) to Makefile. 
(these point to the different config files, which are cofigured to use the appropriate LUA header and libs in the target_binaries directory).
2. make lib #(creates the lfs.so library)
3. make install # copies lfs.so into the appropriate target_binary structure ( note no share lua scripts are used - just a lib binary seems appropriate).
4. check in the target_binary dorectory - the lib will then be copied across by the target_env build process.  