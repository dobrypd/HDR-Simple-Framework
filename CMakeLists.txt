# HDRSimpleFramework
# Author Piotr Dobrowolski
#
# MIT License
#
# Copyright (c) 2017 Piotr Dobrowolski
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT("HDRSimpleFramework")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# C++11
INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
IF(COMPILER_SUPPORTS_CXX11)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSEIF(COMPILER_SUPPORTS_CXX0X)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSE()
    MESSAGE(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
ENDIF()

# All warnings
IF(MSVC)
    IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    ELSE()
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    ENDIF()
ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
ENDIF()

SET(LIBS)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/src/")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

FIND_PACKAGE(OpenCV 3.0 REQUIRED)
FIND_PACKAGE(Boost 1.50 COMPONENTS system filesystem thread REQUIRED)
FIND_PACKAGE(exiv2 REQUIRED)
FIND_PACKAGE(LibRAW REQUIRED)

INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIR} "${EXIV2_INCLUDE_DIR}/exiv2")
INCLUDE_DIRECTORIES(${LIBRAW_INCLUDE_DIR} "${LIBRAW_INCLUDE_DIR}/libraw")

SET(LIBS ${LIBS} ${Boost_LIBRARIES})
SET(LIBS ${LIBS} ${OpenCV_LIBRARIES})
SET(LIBS ${LIBS} ${EXIV2_LIBRARIES})
SET(LIBS ${LIBS} ${LIBRAW_LIBRARIES})

SET(FILES_HXX)
SET(FILES_CPP)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

SET(HDRSimpleFrameworkFILES
        ${FILES_HXX}
        ${FILES_CPP})

ADD_EXECUTABLE(${PROJECT_NAME} ${HDRSimpleFrameworkFILES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${MODULES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})

# Documentation
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/doc/${PROJECT_NAME}.config ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    ADD_CUSTOM_TARGET(doc
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen" VERBATIM
            )
ENDIF(DOXYGEN_FOUND)

