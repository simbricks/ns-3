build_lib_example(
  NAME sequencer-single-switch-example
  SOURCE_FILES sequencer-single.cc
  LIBRARIES_TO_LINK ${libsequencer}
                    ${libsimbricks}
                    ${libbridge}
                    ${libinternet}
                    ${libapplications}
)