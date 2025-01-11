
#ifndef THREEPP_IMGUI_HELPER_HPP
#define THREEPP_IMGUI_HELPER_HPP

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

#include <functional>
#include <threepp/canvas/Monitor.hpp>

// #include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "RobotoRegular.cpp"
#include "RobotoMedium.cpp"
#include "MaterialIcons.cpp"

namespace ImGui {
    ImFont* Medium36;
    ImFont* Medium46;

    ImFont* Bold26;
    ImFont* Bold36;
    ImFont* Bold46;

}

class ImguiContext {

public:
    explicit ImguiContext(void* window, bool dpiAware = false) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_NavEnableGamepad;

        ImPlot::CreateContext();


        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window), true);
#if EMSCRIPTEN
        ImGui_ImplOpenGL3_Init("#version 300 es");
#else
        ImGui_ImplOpenGL3_Init("#version 330");
#endif

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        if (dpiAware) {
            const auto [dpiScaleX, _] = threepp::monitor::contentScale();
            io.FontGlobalScale = dpiScaleX;

            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(dpiScaleX);
        }

        CustomStyleDiscord();
    }

    ImguiContext(ImguiContext&&) = delete;
    ImguiContext(const ImguiContext&) = delete;
    ImguiContext& operator=(const ImguiContext&) = delete;

    void render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        onRender();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    virtual ~ImguiContext() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        ImPlot::DestroyContext();
    }

    void CustomStyleDiscord() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Discord dark theme color palette
        ImVec4 discordBackground = ImVec4(0.137f, 0.149f, 0.165f, 1.00f);  // #23272A
        ImVec4 discordChannelBar = ImVec4(0.196f, 0.212f, 0.235f, 1.00f);  // #2C2F33
        ImVec4 discordChannelSelected = ImVec4(0.227f, 0.247f, 0.275f, 1.00f);  // #393C43
        ImVec4 discordText       = ImVec4(1.000f, 1.000f, 1.000f, 1.00f);  // #FFFFFF
        ImVec4 discordTextDim    = ImVec4(0.608f, 0.627f, 0.655f, 1.00f);  // #99AAB5
        ImVec4 discordBlurple    = ImVec4(0.357f, 0.431f, 0.882f, 1.00f);  // #5865F2

        colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);         // 흰색 텍스트
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.36f, 0.37f, 0.39f, 1.00f);         // 비활성화된 텍스트
        colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);         // 더 어두운 메인 배경
        colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);         // 자식 창 배경
        colors[ImGuiCol_PopupBg]                = ImVec4(0.13f, 0.14f, 0.15f, 0.94f);         // 팝업 배경
        colors[ImGuiCol_Border]                 = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);         // 더 어두운 테두리
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);         // 테두리 그림자
        colors[ImGuiCol_FrameBg]                = ImVec4(0.11f, 0.12f, 0.13f, 1.00f);         // 입력 필드 배경
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);         // 호버된 입력 필드
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);         // 활성화된 입력 필드
        colors[ImGuiCol_TitleBg]                = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);         // 타이틀 바 배경
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);         // 활성화된 타이틀 바
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);         // 접힌 타이틀 바
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);         // 메뉴바 배경
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);         // 스크롤바 배경
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);         // 스크롤바 그랩
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.22f, 0.23f, 0.24f, 1.00f);         // 호버된 스크롤바
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.27f, 0.28f, 0.29f, 1.00f);         // 클릭된 스크롤바
        colors[ImGuiCol_CheckMark]              = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 체크마크 (더 진한 블루퍼플)
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 슬라이더 그랩
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.47f, 0.50f, 0.96f, 1.00f);         // 활성화된 슬라이더
        colors[ImGuiCol_Button]                 = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 버튼
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.47f, 0.50f, 0.96f, 1.00f);         // 호버된 버튼
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.27f, 0.30f, 0.96f, 1.00f);         // 클릭된 버튼
        colors[ImGuiCol_Header]                 = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);         // 헤더
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.22f, 0.23f, 0.24f, 1.00f);         // 호버된 헤더
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.27f, 0.28f, 0.29f, 1.00f);         // 클릭된 헤더
        colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);         // 구분선
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.22f, 0.23f, 0.24f, 1.00f);         // 호버된 구분선
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.27f, 0.28f, 0.29f, 1.00f);         // 클릭된 구분선
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.37f, 0.40f, 0.96f, 0.20f);         // 리사이즈 그립
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.37f, 0.40f, 0.96f, 0.67f);         // 호버된 리사이즈 그립
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.37f, 0.40f, 0.96f, 0.95f);         // 클릭된 리사이즈 그립
        colors[ImGuiCol_Tab]                    = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);         // 탭
        colors[ImGuiCol_TabHovered]             = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);         // 호버된 탭
        colors[ImGuiCol_TabActive]              = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);         // 활성화된 탭
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);         // 포커스 되지 않은 탭
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);         // 포커스 되지 않은 활성 탭
        colors[ImGuiCol_PlotLines]              = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 플롯 라인
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.47f, 0.50f, 0.96f, 1.00f);         // 호버된 플롯 라인
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 히스토그램
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.47f, 0.50f, 0.96f, 1.00f);         // 호버된 히스토그램
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.37f, 0.40f, 0.96f, 0.35f);         // 선택된 텍스트 배경
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.37f, 0.40f, 0.96f, 0.90f);         // 드래그 드롭 타겟
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.37f, 0.40f, 0.96f, 1.00f);         // 네비게이션 하이라이트
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.37f, 0.40f, 0.96f, 0.70f);         // 윈도우 네비게이션 하이라이트
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);         // 윈도우 네비게이션 딤
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);         // 모달 윈도우 딤

        // Adjust style
        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 4.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;
        style.ChildRounding     = 4.0f;
        style.FramePadding      = ImVec2(8, 4);
        style.ItemSpacing       = ImVec2(8, 4);
        style.ItemInnerSpacing  = ImVec2(4, 4);
        style.IndentSpacing     = 21.0f;
        style.ScrollbarSize     = 10.0f;
        style.GrabMinSize       = 8.0f;
        style.TabBarOverlineSize= 0.0f;

        ImGuiIO& io = ImGui::GetIO();
        // Add font RobotoMedium
        float sizePixels = 26.0f;
        //ImFont* fontRobotoMedium = io.Fonts->AddFontFromMemoryCompressedTTF(&RobotoMedium_compressed_data, RobotoMedium_compressed_size, sizePixels);
        ImFont* fontRobotoMedium = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Medium.ttf", sizePixels, NULL, io.Fonts->GetGlyphRangesKorean());
		//io.Fonts->AddFontFromMemoryCompressedTTF(&RobotoMedium_compressed_data, RobotoMedium_compressed_size, sizePixels);
        IM_ASSERT(fontRobotoMedium != nullptr);

        // Merge icons to font RobotoMedium
        ImFontConfig config;
        config.MergeMode = true;
        config.PixelSnapH = true;
        config.GlyphOffset = ImVec2(0.0f, 3.0f); // Offset
        static const ImWchar ranges[] = { 0xe000, 0xf3ff, 0 };
        ImFont* fontMaterialIcons = io.Fonts->AddFontFromMemoryCompressedTTF(&MaterialIcons_compressed_data, MaterialIcons_compressed_size, sizePixels, &config, ranges);
        IM_ASSERT(fontMaterialIcons != nullptr);


        // 폰트 불러오기
        ImGui::Medium36 = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Medium.ttf", 36, NULL, io.Fonts->GetGlyphRangesKorean());
        IM_ASSERT(ImGui::Medium36 != nullptr);
        ImGui::Medium46 = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Medium.ttf", 50, NULL, io.Fonts->GetGlyphRangesKorean());
        IM_ASSERT(ImGui::Medium46 != nullptr);

        ImGui::Bold26 = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Bold.ttf", 26, NULL, io.Fonts->GetGlyphRangesKorean());
        IM_ASSERT(ImGui::Bold26 != nullptr);
        ImGui::Bold36 = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Bold.ttf", 36, NULL, io.Fonts->GetGlyphRangesKorean());
        IM_ASSERT(ImGui::Bold36 != nullptr);
        ImGui::Bold46 = io.Fonts->AddFontFromFileTTF("resource/Fonts/NEXON Lv2 Gothic Bold.ttf", 50, NULL, io.Fonts->GetGlyphRangesKorean());
        IM_ASSERT(ImGui::Bold46 != nullptr);





        // Add font RobotoRegular
        // ImFont* fontRobotoRegular = io.Fonts->AddFontFromMemoryCompressedTTF(&RobotoRegular_compressed_data, RobotoRegular_compressed_size, sizePixels);
        // IM_ASSERT(fontRobotoRegular != nullptr);
        //
        // // Add large font
        // sizePixels = 28.0f;
        // ImFont* fontLarge = io.Fonts->AddFontFromMemoryCompressedTTF(&RobotoMedium_compressed_data, RobotoMedium_compressed_size, sizePixels);
        // IM_ASSERT(fontLarge != nullptr);
        //
        // // Merge large icons to large font
        // ImFontConfig configLarge;
        // configLarge.MergeMode = true;
        // configLarge.PixelSnapH = true;
        // configLarge.GlyphOffset = ImVec2(0.0f, 4.0f); // Offset
        // static const ImWchar rangesLarge[] = { 0xe000, 0xf3ff, 0 };
        // auto iconLarge = io.Fonts->AddFontFromMemoryCompressedTTF(&MaterialIcons_compressed_data, MaterialIcons_compressed_size, sizePixels, &configLarge, rangesLarge);
        // IM_ASSERT(iconLarge != nullptr);
    }

protected:
    virtual void onRender() = 0;
};

class ImguiFunctionalContext: public ImguiContext {

public:
    explicit ImguiFunctionalContext(void* window, std::function<void()> f)
        : ImguiContext(window, false),
          f_(std::move(f)) {}


protected:
    void onRender() override {
        f_();
    }

private:
    std::function<void()> f_;
};

#endif//THREEPP_IMGUI_HELPER_HPP
