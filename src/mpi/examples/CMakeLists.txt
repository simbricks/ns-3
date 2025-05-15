link_libraries(${MPI_CXX_LIBRARIES})

set(base_examples
    simple-distributed
    simple-distributed-mpi-comm
    nms-p2p-nix-distributed
    simple-distributed-empty-node
    dc_fat_mpi
    dc_fat_sim
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
                 mpi-test-fixtures.cc
    LIBRARIES_TO_LINK
      ${libmpi}
      ${libpoint-to-point}
      ${libinternet}
      ${libnix-vector-routing}
      ${libapplications}
  )
endforeach()

build_lib_example(
  NAME third-distributed
  SOURCE_FILES third-distributed.cc
               mpi-test-fixtures.cc
  LIBRARIES_TO_LINK
    ${libmpi}
    ${libpoint-to-point}
    ${libinternet}
    ${libmobility}
    ${libwifi}
    ${libcsma}
    ${libapplications}
)

build_lib_example(
  NAME fct_fat
  SOURCE_FILES fct_fat.cc
  LIBRARIES_TO_LINK ${libsimbricks}
                    ${libbridge}
                    ${libpoint-to-point}
                    ${libinternet}
                    ${libred}
                    ${libapplications}
                    ${libcsma}
)