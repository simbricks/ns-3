build_lib_example(
  NAME simbricks-example
  SOURCE_FILES simbricks-example.cc
  LIBRARIES_TO_LINK ${libsimbricks}
)

build_lib_example(
  NAME simbricks-bridge-example
  SOURCE_FILES simbricks-bridge.cc
  LIBRARIES_TO_LINK ${libsimbricks}
                    ${libbridge}
)

build_lib_example(
  NAME simbricks-dumbbell-example
  SOURCE_FILES simbricks-dumbbell.cc
  LIBRARIES_TO_LINK ${libsimbricks}
                    ${libbridge}
                    ${libpoint-to-point}
                    ${libinternet}
                    ${libred}
)

build_lib_example(
  NAME simbricks-nicif-example
  SOURCE_FILES NicifTwoHost.cc
  LIBRARIES_TO_LINK ${libsimbricks}
                    ${libcore}
                    ${libnetwork}
)

build_lib_example(
  NAME simbricks-netif-example
  SOURCE_FILES NetifTwoHost.cc
  LIBRARIES_TO_LINK ${libsimbricks}
                    ${libcore}
                    ${libnetwork}
)