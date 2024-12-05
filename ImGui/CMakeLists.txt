# ImGui/CMakeLists.txt

set(IMGUI_SOURCES
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_impl_glfw.cpp
        imgui_impl_opengl3.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        implot.cpp
        implot_demo.cpp
        implot_items.cpp
)
set(IMGUI_HEADERS
        imconfig.h
        imgui.h
        imgui_internal.h
        imgui_impl_glfw.h
        imgui_impl_opengl3.h
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        implot.h
        implot_internal.h
)

add_library(ImGui STATIC ${IMGUI_SOURCES})

target_include_directories(ImGui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
