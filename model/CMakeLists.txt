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
SET(mail_srcs
	mail_main.c
		mail_post_office.c
	mail_map.c
	mail.h
	model_config.h
		mail_mailbox.c
)




# Set the executable name here: model
# You will be executing the ./model binary
ADD_EXECUTABLE(mail ${mail_srcs})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(mail ${ROSS_LIBRARIES} ${MPI_C_LIBRARIES})



# Set the executable name here: mail
# You will be executing the ./mail binary
#ADD_EXECUTABLE(mail ${mail_srcs})
#
#TARGET_LINK_LIBRARIES(mail ROSS m)