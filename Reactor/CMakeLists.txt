set(REACTOR_SOURCES
        AEventHandler.cpp
        handler.cpp
        reactor.cpp
        server.cpp
)

add_library(${REACTOR_LIBRARY} STATIC ${REACTOR_SOURCES})
target_include_directories(${REACTOR_LIBRARY} PUBLIC .)
target_link_libraries(${REACTOR_LIBRARY} PUBLIC ${HTTP_LIBRARY} ${CONFIG_LIBRARY})
