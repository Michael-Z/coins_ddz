import qbs

CppApplication {
    files: [
        "src/AccessUserManager.cpp",
        "src/AccessUserManager.h",
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/ClientHandlerProxy.cpp",
        "src/ClientHandlerProxy.h",
        "src/ConnectHandlerProxy.cpp",
        "src/ConnectHandlerProxy.h",
        "src/ConnectHandlerToken.cpp",
        "src/ConnectHandlerToken.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
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