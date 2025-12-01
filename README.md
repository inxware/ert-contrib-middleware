# eRT-contrib-middleware
Contains contributed open surce middleware libraries used for some configirations of 
the inxware eRT (event driven Run-Time). 

https://www.inx-systems.com/platform/

# To Use This Repo to build a platfrom 

First clone the ert-components repo from https://github.com/inxware/ert-components (or the private repo)



# To Add to or Modify This Repository 

cd to inx-build scripts
./build_all [targetname]

where target name is $OS_$ARCH

e.g.

linux_x86

For each component the following build directories are created:

built libs, includes and pkconfigs (where appropriate) in $COMPOMENT/target_lib_builds

The target manefest (compnent support) is created in $COMPOMENT/target_cs_pckages 

An aggregation of $COMPOMENT/target_lib_builds is built in ../ target_libs
This is for building against and contains all normall install info with this prefix.

