import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/DBProxyHandlerProxy.cpp",
        "src/DBProxyHandlerProxy.h",
        "src/DBProxyHandlerToken.cpp",
        "src/DBProxyHandlerToken.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
        "src/RedisManager.cpp",
        "src/RedisManager.h",
        "src/UserRedisServer.cpp",
        "src/UserRedisServer.h",
        "src/hiredis.h",
        "src/server.cpp",
    ]
    consoleApplication: true

    cpp.includePaths: [
        "../../proto",
        "../common",
        "../../core/include",
        "../../libs",
        "src",
        "src/Processer/",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
