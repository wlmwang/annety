
# source files lists.
AUX_SOURCE_DIRECTORY(. SRC_LIST)

# Build the library.
ADD_LIBRARY(annety ${SRC_LIST})
TARGET_LINK_LIBRARIES(annety pthread)

# install the library target.
INSTALL(TARGETS annety 
	ARCHIVE DESTINATION lib
)

# install the headers file.
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/annety/include/ 
	DESTINATION include/annety
)
