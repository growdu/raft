aux_source_directory(. RAFT_SRC_DIR)

add_library(raft SHARED ${RAFT_SRC_DIR})

target_include_directories(raft PRIVATE 
                            ${RAFT_HOME_DIR}/src
                            ${LIBUV_HOME_DIR}/include
                            )

#install
INSTALL(TARGETS raft
        RUNTIME DESTINATION {CMAKE_PREFIX_PATH}/bin
        LIBRARY DESTINATION {CMAKE_PREFIX_PATH}/lib
        ARCHIVE DESTINATION {CMAKE_PREFIX_PATH}/lib        
)
INSTALL(DIRECTORY ${RAFT_HOME_DIR}/include DESTINATION {CMAKE_PREFIX_PATH})