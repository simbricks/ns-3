check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/e2e-cc-examples-test-suite.cc
        )
endif()

set(simbricks_path $ENV{SIMBRICKS_PATH})

include_directories(${simbricks_path}/lib)

build_lib(
    LIBNAME e2e-cc
    SOURCE_FILES model/e2e-config.cc
                 model/e2e-component.cc
                 model/e2e-application.cc
                 model/e2e-host.cc
                 model/e2e-topology.cc
                 model/e2e-probe.cc
                 helper/e2e-cc-helper.cc
    HEADER_FILES model/e2e-config.h
                 model/e2e-component.h
                 model/e2e-application.h
                 model/e2e-host.h
                 model/e2e-topology.h
                 model/e2e-probe.h
                 helper/e2e-cc-helper.h
    LIBRARIES_TO_LINK ${libcore}
                      ${libsimbricks}
                      ${libapplications}
    TEST_SOURCES test/e2e-cc-test-suite.cc
                 ${examples_as_tests_sources}
)

