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
    model/simbricks.cc
    model/simbricks-adapter.cc
    model/simbricks-adapter-nicif.cc
    model/simbricks-nicif.cc
  HEADER_FILES
    model/simbricks.h
    model/simbricks-adapter.h
    model/simbricks-adapter-nicif.h
    model/simbricks-nicif.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
)