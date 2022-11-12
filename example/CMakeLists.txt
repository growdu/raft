link_directories(
    ${LIBUV_HOME_DIR}/lib
)
add_executable(cluster cluster.c)
target_include_directories(cluster PRIVATE 
                            ${RAFT_HOME_DIR}/src
                            ${LIBUV_HOME_DIR}/include
                            )
target_link_libraries(cluster raft libuv liblz4)
set_target_properties(cluster PROPERTIES INSTALL_RPATH_USE_LINK_PATH true)

add_executable(server server.c)
target_include_directories(server PRIVATE 
                            ${RAFT_HOME_DIR}/src
                            ${LIBUV_HOME_DIR}/include
                            )
target_link_libraries(server raft libuv liblz4)
set_target_properties(server PROPERTIES INSTALL_RPATH_USE_LINK_PATH true)

#install
INSTALL(TARGETS cluster server
        RUNTIME DESTINATION {CMAKE_PREFIX_PATH}/bin
        LIBRARY DESTINATION {CMAKE_PREFIX_PATH}/lib
        ARCHIVE DESTINATION {CMAKE_PREFIX_PATH}/lib          
)
