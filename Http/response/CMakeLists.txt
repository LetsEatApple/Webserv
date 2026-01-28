set(RESPONSE_SOURCES
        Response.cpp
)

add_library(${RESPONSE_LIBRARY} STATIC
        ${RESPONSE_SOURCES}
)
target_include_directories(${RESPONSE_LIBRARY} PUBLIC .)
target_link_libraries(${RESPONSE_LIBRARY} PUBLIC ${REQUEST_LIBRARY})
