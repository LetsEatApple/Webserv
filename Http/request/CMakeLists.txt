set(REQUEST_SOURCES
        Headers.cpp
        Request.cpp
)

add_library(${REQUEST_LIBRARY} STATIC
        ${REQUEST_SOURCES}
)
target_include_directories(${REQUEST_LIBRARY} PUBLIC .)
