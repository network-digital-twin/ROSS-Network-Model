#INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})
### Must include binary dir for config.h
#
### Model Version
#LIST(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
##INCLUDE(GetGitRevisionDescription)
##GET_GIT_HEAD_REVISION(GIT_RESPEC_MODEL GIT_SHA1_MODEL)
#
### Build Configuration Header File
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
#CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/model_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/model_config.h @ONLY)
#SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/model_config.h.in PROPERTIES GENERATED FALSE)
#SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/model_config.h PROPERTIES GENERATED TRUE)

# All source files for the model

SET(network_srcs
		network_main.c
		network_switch.c
		network_map.c
		network.h
		model_config.h
		network_terminal.c
		qos/queue_fifo.c
		qos/meter.c
		qos/shaper.c
		qos/scheduler/strict_priority.c
		util/parser.h
		util/message.c
		util/conversion.c
)




# Set the executable name here: network
# You will be executing the ./network binary
ADD_EXECUTABLE(network ${network_srcs})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(network ${ROSS_LIBRARIES} ${MPI_C_LIBRARIES})
