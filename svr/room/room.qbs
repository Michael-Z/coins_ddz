import qbs

CppApplication {
    files: [
        "src/AppServer.cpp",
        "src/AppServer.h",
        "src/GameSvrdManager.cpp",
        "src/GameSvrdManager.h",
        "src/Processer/CommonClienthandler.cpp",
        "src/Processer/CommonClienthandler.h",
        "src/ProcesserManager.cpp",
        "src/ProcesserManager.h",
        "src/RoomHandlerProxy.cpp",
        "src/RoomHandlerProxy.h",
        "src/RoomHandlerToken.cpp",
        "src/RoomHandlerToken.h",
        "src/TableManager.cpp",
        "src/TableManager.h",
        "src/ZoneTableManager.cpp",
        "src/ZoneTableManager.h",
        "src/QuickMatchManager.h",
        "src/QuickMatchManager.cpp",
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
