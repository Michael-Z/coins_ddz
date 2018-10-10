    cmake_minimum_required(VERSION 2.8)
	SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fPIC -Wno-invalid-offsetof")
	#source directory，源文件目录
	aux_source_directory(. SOURCE_FILES)
	aux_source_directory(Processer PROCESSER_SOURCE_FILES)
	list(APPEND SOURCE_FILES ${PROCESSER_SOURCE_FILES})
	 
	 message(STATUS "-----" ${PROJECT_SOURCE_DIR})

     INCLUDE_DIRECTORIES(                          #相关头文件的目录
		/usr/local/include
		../../../proto
		../../../core/include
		../../common
		./Processer
		./
     )
	
    LINK_DIRECTORIES(                                  #相关库文件的目录
		/usr/local/lib
		${PROJECT_SOURCE_DIR}/libs
    )


    add_executable (${PROJECT_NAME}RobotSvrd ${SOURCE_FILES})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME}RobotSvrd  libcommon.a libproto.a libreactor.a libprotobuf.a libhiredis.a)   			#依赖的库文件
	
	INSTALL(TARGETS ${PROJECT_NAME}RobotSvrd RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/svr/teabar/bin)

   