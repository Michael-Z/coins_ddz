import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/DBManager.cpp",
        "src/DBManager.h",
        "src/DataBase.cpp",
        "src/DataBase.h",
        "src/DataBaseHandler.cpp",
        "src/DataBaseHandler.h",
        "src/LogHandlerProxy.cpp",
        "src/LogHandlerProxy.h",
        "src/LogHandlerToken.cpp",
        "src/LogHandlerToken.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
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