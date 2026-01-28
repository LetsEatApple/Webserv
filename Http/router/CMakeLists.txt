set(SERVER_SOURCES
        Router.cpp
)

add_library(${SERVER_LIBRARY} STATIC
        ${SERVER_SOURCES}
)
target_include_directories(${SERVER_LIBRARY} PUBLIC .)
target_link_libraries(${SERVER_LIBRARY} PUBLIC ${RESPONSE_LIBRARY} ${REQUEST_LIBRARY})
