import qbs

CppApplication {
    files: [
        "include/DecoderBase.h",
        "include/DefaultStreamDecoder.h",
        "include/FrameServer.h",
        "include/HandlerProxyBasic.h",
        "include/Lock.h",
        "include/NewReactor.h",
        "include/Reactor.h",
        "include/TCPSocketHandler.h",
        "include/TCPSocketServerBasic.h",
        "include/TCP_Handler_Base.h",
        "include/TCP_Server_Base.h",
        "include/Timer_Handler_Base.h",
        "include/cache.h",
        "include/clib_log.h",
        "include/global.h",
        "include/list.h",
        "include/memcheck.h",
        "include/mempool.h",
        "include/myepoll.h",
        "include/net.h",
        "include/poller.h",
        "include/singleton.h",
        "include/socket.h",
        "include/timerlist.h",
        "include/timestamp.h",
        "src/FrameServer.cpp",
        "src/HandlerProxyBasic.cpp",
        "src/NewReactor.cpp",
        "src/Reactor.cpp",
        "src/TCPSocketHandler.cpp",
        "src/TCPSocketServerBasic.cpp",
        "src/Timer_Handler_Base.cpp",
        "src/cache.cpp",
        "src/clib_log.cpp",
        "src/global.cpp",
        "src/mempool.cpp",
        "src/net.cpp",
        "src/poller.cpp",
        "src/timerlist.cpp",
    ]
    consoleApplication: true

    cpp.includePaths: [
        "proto",
        "include",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}