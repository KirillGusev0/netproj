add_executable(gen-app
    main.cpp
    server.cpp
    client.cpp
)

target_link_libraries(gen-app PRIVATE pthread)