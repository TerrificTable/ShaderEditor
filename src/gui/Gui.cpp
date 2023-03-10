#include "Gui.h"

#include "themes.h"
#include "../utils.h"
#include "../info.h"
#include "../imgui/imgui_toggle.h"
#include "font_roboto.hpp"
#include "Animator.h"


#define DEBUG


Gui::Gui(int width, int height) {
    WIDTH = width;
    HEIGHT = height;
}

Gui::~Gui() {
    Gui::DestroyImGui();
    Gui::DestroyOpenGlWindow();
}




void GlfwErrorCallback(int error, const char* description) {
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

void Gui::SetupOpenGl() {
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize glfw");
#ifdef DEBUG
    std::cout << "Initialized GLFW" << std::endl;
#endif

    Gui::glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

void Gui::CreateOpenGlWindow(const char *name) {
    window = glfwCreateWindow(WIDTH, HEIGHT, name, nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to create window");
#ifdef DEBUG
    std::cout << "Created GLFW Window" << std::endl;
#endif

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }
#ifdef DEBUG
    std::cout << "Initialized GLAD" << std::endl;
#endif

    glfwSwapInterval(1);
}

void Gui::DestroyOpenGlWindow() noexcept {
    glfwDestroyWindow(window);
    glfwTerminate();
}













void Gui::CreateImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    ImGui::StyleColorsDark();
    Theme::PhocosGreen();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    io.Fonts->AddFontDefault();
    ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(roboto_compressed_data, roboto_compressed_size, 16);
    assert(font != nullptr && "Failed to load roboto font");
    io.FontDefault = font;

    show_shader_window = false;

#ifdef DEBUG
    std::cout << "Created ImGui window" << std::endl;
#endif
}

void Gui::DestroyImGui() noexcept {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}









void Gui::BeginRenderer() noexcept {
    auto lang = TextEditor::LanguageDefinition::GLSL();
    editor.SetLanguageDefinition(lang);
    editor.SetPalette(TextEditor::GetDarkPalette());

    if (fileExists("shaders/default.fsh"))
        editor.SetText(readFile("shaders/default.fsh"));
    else
        editor.SetText("#version 330\n\nuniform float time;\nuniform vec2 resolution;\nuniform vec2 mouse;\n\nvoid main() {\n\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n}");


    tr_a =  1.0f; tr_b =  1.0f; tr_c = 0.0f;
    br_a =  1.0f; br_b = -1.0f; br_c = 0.0f;
    bl_a = -1.0f; bl_b = -1.0f; bl_c = 0.0f;
    tl_a = -1.0f; tl_b =  1.0f; tl_c = 0.0f;

    float r_tr[] = { tr_a, tr_b, tr_c };
    float r_br[] = { br_a, br_b, br_c };
    float r_bl[] = { bl_a, bl_b, bl_c };
    float r_tl[] = { tl_a, tl_b, tl_c };
    Rectangle rectangle(r_tl, r_tr, r_bl, r_br);

    shader = {editor.GetText()};
    while (!glfwWindowShouldClose(Gui::window) && !exit) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        WIDTH = display_w;
        HEIGHT = display_h;

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        if (Gui::Render() != 0)
            break;
        ImGui::Render();

        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        float _r_tr[] = { tr_a, tr_b, tr_c };
        float _r_br[] = { br_a, br_b, br_c };
        float _r_bl[] = { bl_a, bl_b, bl_c };
        float _r_tl[] = { tl_a, tl_b, tl_c };
        rectangle = {_r_tl, _r_tr, _r_bl, _r_br};

        shader.use();
        glUniform1f(glGetUniformLocation(shader.programID, "time"), glfwGetTime());
        glUniform2f(glGetUniformLocation(shader.programID, "mouse"), mouse_x, mouse_y);
        glUniform2f(glGetUniformLocation(shader.programID, "resolution"), display_w, display_h);
        rectangle.Draw();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    rectangle.Delete();
}




int Gui::Render() noexcept {
//    ImGui::ShowDemoWindow();

    static bool show_editor = true;
    static bool show_debug = false;

    static int shader_width = 1280;
    static int shader_height = 720;
//    static bool move_outside_window = false;

    if (show_shader_window) {
        ImGui::SetNextWindowSize(ImVec2(shader_width, shader_height));
        ImGui::Begin("Shader", &show_shader_window); {
            // TODO: Fix rendering shader to fbo and displaying fbo as imgui_old image
            //       also create header and cpp file for FBO

//            ImGui::Image(reinterpret_cast<ImTextureID>(shader.texture2d), ImVec2(shader_width, shader_height));

            ImGui::End();
        }
    }

    if (show_editor) {
        ImGui::Begin("TextEditor", &show_editor, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl-S")) {
                    auto textToSave = editor.GetText();
                    shader = {textToSave};
                }
                if (ImGui::MenuItem("Quit", "Alt-F4"))
                    return -1;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl-Z", nullptr, editor.CanUndo()))
                    editor.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, editor.CanRedo()))
                    editor.Redo();

                ImGui::Separator();

                if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                    editor.Copy();
                if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, editor.HasSelection()))
                    editor.Cut();
                if (ImGui::MenuItem("Delete", "Del", nullptr, editor.HasSelection()))
                    editor.Delete();
                if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, ImGui::GetClipboardText() != nullptr))
                    editor.Paste();

                ImGui::Separator();

                if (ImGui::MenuItem("Select all", nullptr, nullptr))
                    editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Dark palette"))
                    editor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette"))
                    editor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette"))
                    editor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        editor.Render("TextEditor");
        ImGui::End();
    }

    if (show_debug) {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        static int location = 0;

        if (location >= 0) {
            const float PAD = 10.0f;
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos = viewport->WorkPos;
            ImVec2 work_size = viewport->WorkSize;
            ImVec2 window_pos, window_pos_pivot;
            window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
            window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
            window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
            window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            window_flags |= ImGuiWindowFlags_NoMove;
        } else if (location == -2) {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            window_flags |= ImGuiWindowFlags_NoMove;
        }

        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("Debug", &show_debug, window_flags);
        ImGui::Text("Version: %s", Info::Version.c_str());
        ImGui::Text("OpenGL: %s", glfwGetVersionString());

        ImGui::Separator();
        ImGui::Text("Window: %dx%d", WIDTH, HEIGHT);
        ImGui::Text("Time: %f", glfwGetTime());
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom",       nullptr, location == -1)) location = -1;
            if (ImGui::MenuItem("Center",       nullptr, location == -2)) location = -2;
            if (ImGui::MenuItem("Top-left",     nullptr, location == 0)) location = 0;
            if (ImGui::MenuItem("Top-right",    nullptr, location == 1)) location = 1;
            if (ImGui::MenuItem("Bottom-left",  nullptr, location == 2)) location = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, location == 3)) location = 3;
            if (show_debug && ImGui::MenuItem("Close")) show_debug = false;
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    ImGui::Begin("Options"); {
        if (ImGui::TreeNode("Position")) {
            ImGui::SliderFloat("Top Right X", &tr_a, -1.0f, 1.0f);
            ImGui::SliderFloat("Top Right Y", &tr_b, -1.0f, 1.0f);
            ImGui::Separator();

            ImGui::SliderFloat("Top Left X", &tl_a, -1.0f, 1.0f);
            ImGui::SliderFloat("Top Left Y", &tl_b, -1.0f, 1.0f);
            ImGui::Separator();

            ImGui::SliderFloat("Bottom Right X", &br_a, -1.0f, 1.0f);
            ImGui::SliderFloat("Bottom Right Y", &br_b, -1.0f, 1.0f);
            ImGui::Separator();

            ImGui::SliderFloat("Bottom Left X", &bl_a, -1.0f, 1.0f);
            ImGui::SliderFloat("Bottom Left Y", &bl_b, -1.0f, 1.0f);

            ImGui::TreePop();
        }
        ImGui::Separator();

        if (show_shader_window) {
            if (ImGui::TreeNode("Shader Window Scale")) {
                ImGui::SliderInt("Width", &shader_width, 1, WIDTH);
                ImGui::SliderInt("Height", &shader_height, 1, HEIGHT);

                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        static int style_idx = 0;
        if (ImGui::Combo("Style", &style_idx, "Phocus Green\0Dark\0Light\0Classic\0Maya\0Monochrome\0The_0n3\0ModernDarkTheme\0EmbraceTheDarkness\0")) {
            switch (style_idx) {
                case 0: Theme::PhocosGreen(); break;
                case 1: ImGui::StyleColorsDark(); break;
                case 2: ImGui::StyleColorsLight(); break;
                case 3: ImGui::StyleColorsClassic(); break;
                case 4: Theme::Maya(); break;
                case 5: Theme::Monochrome(); break;
                case 6: Theme::The_0n3(); break;
                case 7: Theme::ModernDarkTheme(); break;
                case 8: Theme::EmbraceTheDarkness(); break;
            }
        }
        ImGui::Separator();

        const ImVec4 green(0.35f, 0.78f, 0.49f, 1.0f);
        const ImVec4 green_hover(0.34f, 0.84f, 0.49f, 1.0f);
        const ImVec4 salmon(1.0f, 0.43f, 0.35f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, green);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, green_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, salmon);

        static ImGuiToggleConfig config;
        config.AnimationDuration = 0.25f;
        config.Flags |= ImGuiToggleFlags_Animated;

        ImGui::Toggle("Debug Info", &show_debug, config);
        ImGui::Toggle("Text Editor", &show_editor, config);
        ImGui::Toggle("Shader in imgui window", &show_shader_window, config);
        ImGui::PopStyleColor(3);

        ImGui::End();
    }

    return 0;
}

