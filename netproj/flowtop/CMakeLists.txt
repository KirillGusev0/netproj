add_executable(flowtop
    main.cpp
    flow_monitor.cpp
)

target_link_libraries(flowtop PRIVATE pcap pthread)
