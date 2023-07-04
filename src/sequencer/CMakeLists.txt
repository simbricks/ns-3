build_lib(
  LIBNAME sequencer
  SOURCE_FILES
    model/sequencer.cc
    helper/sequencer-helper.cc
  HEADER_FILES
    model/sequencer.h
    helper/sequencer-helper.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
  TEST_SOURCES
    test/sequencer-test-suite.cc
)