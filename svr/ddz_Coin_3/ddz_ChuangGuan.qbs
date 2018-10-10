import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/GameHandlerProxy.cpp",
        "src/GameHandlerProxy.h",
        "src/GameHandlerToken.cpp",
        "src/GameHandlerToken.h",
        "src/PokerLogic.cpp",
        "src/PokerLogic.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
        "src/Table.cpp",
        "src/Table.h",
        "src/TableManager.cpp",
        "src/TableManager.h",
        "src/TableModle.cpp",
        "src/TableModle.h",
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
