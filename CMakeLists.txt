cmake_minimum_required(VERSION 3.21)
project(TokenVault VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt 6.8 is the target; 6.5+ is accepted so distro-packaged Qt can still build it.
set(TV_QT_MIN "6.5" CACHE STRING "Minimum Qt version (project targets 6.8)")
find_package(Qt6 ${TV_QT_MIN} REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(OpenSSL 1.1.1 REQUIRED)

option(TOKENVAULT_BUILD_TESTS "Build crypto/vault tests" ON)

# ---- core library (no GUI) --------------------------------------------------
add_library(tvcore STATIC
    src/crypto.cpp  src/crypto.h
    src/totp.cpp    src/totp.h
    src/vault.cpp   src/vault.h
)
target_include_directories(tvcore PUBLIC src)
target_link_libraries(tvcore PUBLIC Qt6::Core OpenSSL::Crypto)
if(WIN32)
    target_link_libraries(tvcore PUBLIC crypt32 ws2_32 advapi32 user32)
endif()

# ---- application -------------------------------------------------------------
qt_add_executable(TokenVault WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/theme.cpp        src/theme.h
    src/uihelpers.cpp    src/uihelpers.h
    src/qrimage.cpp      src/qrimage.h
    third_party/qrcodegen/qrcodegen.cpp
    src/icons.cpp        src/icons.h
    src/busy.h
    src/authdialogs.cpp  src/authdialogs.h
    src/editdialogs.cpp  src/editdialogs.h
    src/tokendetail.cpp  src/tokendetail.h
    src/mainwindow.cpp   src/mainwindow.h
)
target_include_directories(TokenVault PRIVATE third_party/qrcodegen)
target_link_libraries(TokenVault PRIVATE tvcore Qt6::Gui Qt6::Widgets Qt6::Concurrent)
set_target_properties(TokenVault PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "TokenVault"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.tokenvault"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

# ---- tests -------------------------------------------------------------------
if(TOKENVAULT_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 ${TV_QT_MIN} REQUIRED COMPONENTS Test)
    qt_add_executable(tst_vault tests/tst_vault.cpp)
    target_link_libraries(tst_vault PRIVATE tvcore Qt6::Test)
    add_test(NAME tst_vault COMMAND tst_vault)
endif()

# ---- install / packaging -----------------------------------------------------
include(GNUInstallDirs)
install(TARGETS TokenVault
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/tokenvault.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/tokenvault.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
