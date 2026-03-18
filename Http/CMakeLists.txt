set(HTTP_SOURCES
        request/Headers.cpp
        request/Request.cpp
        response/Response.cpp
        response/ResponseDispatcher.cpp
        response/MimeTypes.cpp
		response/Multipart.cpp
)

add_library(${HTTP_LIBRARY} STATIC ${HTTP_SOURCES})
target_include_directories(${HTTP_LIBRARY} PUBLIC request response)
target_link_libraries(${HTTP_LIBRARY} PUBLIC ${CONFIG_LIBRARY})
