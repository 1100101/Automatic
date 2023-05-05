set(common_srcs ${CMAKE_SOURCE_DIR}/src/output.c
                ${CMAKE_SOURCE_DIR}/src/memwatch.c
                ${CMAKE_SOURCE_DIR}/src/utils.c)

set(common_includes "${CMAKE_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}")

add_executable(list_test ${common_srcs} ${CMAKE_SOURCE_DIR}/src/list.c list_test.c)
target_include_directories(list_test PRIVATE PRIVATE ${common_includes})
add_test(NAME list_test COMMAND list_test)

add_executable(base64_test ${common_srcs} "${CMAKE_SOURCE_DIR}/src/base64.c" base64_test.c)
target_include_directories(base64_test PRIVATE ${common_includes})
add_test(NAME base64_test COMMAND base64_test)

add_executable(regex_test ${common_srcs} "${CMAKE_SOURCE_DIR}/src/regex.c" regex_test.c)
target_include_directories(regex_test PRIVATE ${common_includes})
target_link_libraries(regex_test PRIVATE PCRE::PCRE)
add_test(NAME regex_test COMMAND regex_test)

add_executable(http_test ${common_srcs}
                        "${CMAKE_SOURCE_DIR}/src/file.c"
                        "${CMAKE_SOURCE_DIR}/src/regex.c"
                        "${CMAKE_SOURCE_DIR}/src/urlcode.c"
                        "${CMAKE_SOURCE_DIR}/src/web.c"
                        http_test.c)
target_include_directories(http_test PRIVATE ${common_includes})
target_link_libraries(http_test PRIVATE CURL::libcurl PCRE::PCRE)
add_test(NAME http_test COMMAND http_test)

add_executable(json_test ${common_srcs}
                        "${CMAKE_SOURCE_DIR}/src/base64.c"
                        "${CMAKE_SOURCE_DIR}/src/json.c"
                        "${CMAKE_SOURCE_DIR}/src/regex.c"
                        json_test.c
                        )
target_include_directories(json_test PRIVATE ${common_includes})
target_link_libraries(json_test PRIVATE PCRE::PCRE)
add_test(NAME json_test COMMAND json_test)

add_executable(parser_test ${common_srcs}
                           "${CMAKE_SOURCE_DIR}/src/config_parser.c"
                           "${CMAKE_SOURCE_DIR}/src/filters.c"
                           "${CMAKE_SOURCE_DIR}/src/list.c"
                           "${CMAKE_SOURCE_DIR}/src/regex.c"
                           "${CMAKE_SOURCE_DIR}/src/rss_feed.c"
                           parser_test.c
                           )
target_include_directories(parser_test PRIVATE ${common_includes})
target_link_libraries(parser_test PRIVATE PCRE::PCRE)
add_test(NAME parser_test COMMAND parser_test)

add_executable(utils_test ${common_srcs} utils_test.c)
target_include_directories(utils_test PRIVATE ${common_includes})
add_test(NAME utils_test COMMAND utils_test)

add_executable(pushover_test ${common_srcs}
                            "${CMAKE_SOURCE_DIR}/src/pushover.c"
                            "${CMAKE_SOURCE_DIR}/src/regex.c"
                            "${CMAKE_SOURCE_DIR}/src/urlcode.c"
                            "${CMAKE_SOURCE_DIR}/src/web.c"
                            pushover_test.c
                            )

target_include_directories(pushover_test PRIVATE ${common_includes})
target_link_libraries(pushover_test PRIVATE CURL::libcurl PCRE::PCRE)
add_test(NAME pushover_test COMMAND pushover_test)

add_executable(prowl_test ${common_srcs}
                        "${CMAKE_SOURCE_DIR}/src/prowl.c"
                        "${CMAKE_SOURCE_DIR}/src/regex.c"
                        "${CMAKE_SOURCE_DIR}/src/urlcode.c"
                        "${CMAKE_SOURCE_DIR}/src/web.c"
                        prowl_test.c
                        )
target_include_directories(prowl_test PRIVATE ${common_includes})
target_link_libraries(prowl_test PRIVATE CURL::libcurl PCRE::PCRE)
add_test(NAME prowl_test COMMAND prowl_test)
