
target_include_directories(librsp PUBLIC include)

target_sources(librsp PRIVATE
    src/resp.c
    src/stringbuilder.c
    src/hashtable.c
    src/command.c
    src/sock.c
)

target_sources(rsp PRIVATE
    src/main.c
)