# Scan list of the source files.
AUX_SOURCE_DIRECTORY(. SRC_LIST)

# Build the static library.
ADD_LIBRARY(annety STATIC ${SRC_LIST})
TARGET_LINK_LIBRARIES(annety pthread)

# Install the library targets.
INSTALL(TARGETS annety 
	ARCHIVE DESTINATION lib
)
