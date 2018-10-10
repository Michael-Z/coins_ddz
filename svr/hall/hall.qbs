import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/HallHandlerProxy.cpp",
        "src/HallHandlerProxy.h",
        "src/HallHandlerToken.cpp",
        "src/HallHandlerToken.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
        "src/RecordRedisManager.cpp",
        "src/RecordRedisManager.h",
        "src/RecordRedisServer.cpp",
        "src/RecordRedisServer.h",
        "src/ReplayCodeManager.cpp",
        "src/ReplayCodeManager.h",
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