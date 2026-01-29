set(REACTOR_SOURCES
        AEventHandler.cpp
        handler.cpp
        reactor.cpp
        server.cpp
)

add_library(${REACTOR_LIBRARY} STATIC ${REACTOR_SOURCES})
target_include_directories(${REACTOR_LIBRARY} PUBLIC .)
