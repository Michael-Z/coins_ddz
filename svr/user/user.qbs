import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
        "src/UserHandlerProxy.cpp",
        "src/UserHandlerProxy.h",
        "src/UserHandlerToken.cpp",
        "src/UserHandlerToken.h",
        "src/UserManager.cpp",
        "src/UserManager.h",
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
