set(HTTP_SOURCES
        request/Headers.cpp
        request/Request.cpp
        response/Response.cpp
        router/ReturnDirective.cpp
        router/MimeTypes.cpp
        router/StaticFile.cpp
)

add_library(${HTTP_LIBRARY} STATIC ${HTTP_SOURCES})
target_include_directories(${HTTP_LIBRARY} PUBLIC request response router)
target_link_libraries(${HTTP_LIBRARY} PUBLIC ${CONFIG_LIBRARY})
