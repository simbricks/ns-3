build_lib(
  LIBNAME red
  SOURCE_FILES
    model/dev-red-queue.cc
    model/dev-red-queue-ppp.cc
  HEADER_FILES
    model/dev-red-queue.h
    model/dev-red-queue-ppp.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${libnetwork}
                    ${libpoint-to-point}
)