SET(KFILES_HXX
    ${KFILES_HXX}
    ${CMAKE_CURRENT_SOURCE_DIR}/LuminanceProcessor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HDRCreator.hpp
    PARENT_SCOPE
   )
SET(KFILES_CPP
    ${KFILES_CPP}
    ${CMAKE_CURRENT_SOURCE_DIR}/LuminanceProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HDRCreator.cpp
    PARENT_SCOPE
   )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
