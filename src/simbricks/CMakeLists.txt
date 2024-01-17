set(simbricks_path $ENV{SIMBRICKS_PATH})

include_directories(${simbricks_path}/lib)
link_libraries(
  ${simbricks_path}/lib/simbricks/nicif/libnicif.a
  ${simbricks_path}/lib/simbricks/network/libnetwork.a
  ${simbricks_path}/lib/simbricks/base/libbase.a
)

build_lib(
  LIBNAME simbricks
  SOURCE_FILES
    model/simbricks-base.cc
    model/simbricks-initmgr.cc
    model/simbricks-netdev.cc
  HEADER_FILES
    model/simbricks-base.h
    model/simbricks-initmgr.h
    model/simbricks-netdev.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
)