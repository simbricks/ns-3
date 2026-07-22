link_libraries(
  libnicif.a
  libnetwork.a
  libbase.a
)

build_lib(
  LIBNAME simbricks
  SOURCE_FILES
    model/simbricks-base.cc
    model/simbricks-initmgr.cc
    model/simbricks-netdev.cc
    model/simbricks-trunk.cc
  HEADER_FILES
    model/simbricks-base.h
    model/simbricks-initmgr.h
    model/simbricks-netdev.h
    model/simbricks-trunk.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
)