# libs/skplayer_ui/CMakeLists.txt

add_library(skplayer_ui STATIC
    src/VideoContainer.cpp
    # Headers (for IDE support)
    include/skplayer_ui/VideoContainer.h
    include/skplayer_ui/ThemeConstants.h
)

# Prevent library from being included in iOS archive
set_target_properties(skplayer_ui PROPERTIES
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
)

add_library(skplayer_ui::skplayer_ui ALIAS skplayer_ui)

target_compile_features(skplayer_ui PUBLIC cxx_std_17)

target_include_directories(skplayer_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/skia
)

target_link_libraries(skplayer_ui
    PUBLIC
        skia
)
