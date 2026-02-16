set(HTTP_SOURCES
        request/Headers.cpp
        request/Request.cpp
        response/Response.cpp
        router/Router.cpp
        router/ReturnDirective.cpp
        routes/routes.cpp
)

add_library(${HTTP_LIBRARY} STATIC ${HTTP_SOURCES})
target_include_directories(${HTTP_LIBRARY} PUBLIC request response router routes)
target_link_libraries(${HTTP_LIBRARY} PUBLIC ${CONFIG_LIBRARY})
