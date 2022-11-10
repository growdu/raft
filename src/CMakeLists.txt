aux_source_directory(. RAFT_SRC_DIR)

add_library(raft SHARED ${RAFT_SRC_DIR})

target_include_directories(raft ${RAFT_HOME_DIR}/src)

target_link_directories(raft)