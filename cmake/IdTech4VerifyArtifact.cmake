if(NOT DEFINED ARTIFACT)
    message(FATAL_ERROR "ARTIFACT is required.")
endif()
if(NOT DEFINED ARTIFACT_NAME)
    set(ARTIFACT_NAME "${ARTIFACT}")
endif()

if(NOT EXISTS "${ARTIFACT}")
    message(FATAL_ERROR "${ARTIFACT_NAME} artifact does not exist: ${ARTIFACT}")
endif()

file(SIZE "${ARTIFACT}" _artifact_size)
if(_artifact_size LESS 1)
    message(FATAL_ERROR "${ARTIFACT_NAME} artifact is empty: ${ARTIFACT}")
endif()

message(STATUS "${ARTIFACT_NAME} artifact verified: ${ARTIFACT} (${_artifact_size} bytes)")
