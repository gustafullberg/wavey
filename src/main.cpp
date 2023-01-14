#include <SDL2/SDL.h>
#include "audio_system.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "renderer.hpp"
#include "state.hpp"

float scroll_value = 0.0f;
float scroll_max = 0.0f;
float status_bar_height = 0.0f;
float mouse_x = 0.0f;
float mouse_y = 0.0f;
bool mouse_down = false;

int main(int argc, char** argv) {
    AudioSystem audio;
    State state(&audio);
    for (int i = 1; i < argc; i++) {
        state.LoadFile(argv[i]);
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* window =
        SDL_CreateWindow("Wavey", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    Renderer* renderer = Renderer::Create();

    bool view_spectrogram = false;
    bool view_bark_scale = false;
    bool follow_playback = false;

    int win_width = 1;

    bool run = true;
    while (run) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    run = false;
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        run = false;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (!io.WantCaptureMouse) {
                        mouse_x = std::max(
                            std::min(static_cast<float>(event.motion.x) / io.DisplaySize.x, 1.0f),
                            0.0f);
                        mouse_y = std::max(std::min(static_cast<float>(event.motion.y) /
                                                        (io.DisplaySize.y - status_bar_height),
                                                    1.0f),
                                           0.0f);

                        if (mouse_down) {
                            ZoomWindow& z = state.zoom_window;
                            const float time = z.GetTime(mouse_x);
                            const float dt = std::fabs(time - state.Cursor());
                            // Mouse needs to move at least one pixel to count as a interval
                            // selection.
                            if (dt >= (z.Right() - z.Left()) / io.DisplaySize.x) {
                                state.SetSelection(time);
                            }
                        }

                        state.SetSelectedTrack(state.zoom_window.GetTrack(mouse_y));
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (event.button.clicks == 1) {
                            mouse_down = true;
                            const float time = state.zoom_window.GetTime(mouse_x);
                            state.SetCursor(time);
                        } else if (event.button.clicks == 2) {
                            if (state.SelectedTrack() && state.GetSelectedTrack().audio_buffer) {
                                state.SetCursor(0.f);
                                state.SetSelection(
                                    state.GetSelectedTrack().audio_buffer->Duration());
                            }
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = false;
                        state.FixSelection();
                    }
                    break;

                case SDL_MOUSEWHEEL: {
                    const uint8_t* keyboard = SDL_GetKeyboardState(nullptr);
                    const bool ctrl = keyboard[SDL_SCANCODE_LCTRL] || keyboard[SDL_SCANCODE_RCTRL];
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    if (event.wheel.y > 0) {
                        if (ctrl) {
                            state.zoom_window.ZoomInVertical();
                        } else {
                            state.zoom_window.ZoomIn(static_cast<float>(x) / win_width);
                        }
                    } else if (event.wheel.y < 0) {
                        if (ctrl) {
                            state.zoom_window.ZoomOutVertical();
                        } else {
                            state.zoom_window.ZoomOut(static_cast<float>(x) / win_width);
                        }
                    }
                    if (event.wheel.x < 0) {
                        state.zoom_window.PanLeft();
                    } else if (event.wheel.x > 0) {
                        state.zoom_window.PanRight();
                    }
                    break;
                }

                case SDL_KEYDOWN: {
                    SDL_Keycode key = event.key.keysym.sym;
                    bool shift = event.key.keysym.mod & KMOD_SHIFT;
                    bool ctrl = event.key.keysym.mod & KMOD_CTRL;

                    // Quit.
                    if (key == SDLK_q && ctrl) {
                        run = false;
                    }

                    // Open Files.
                    if (key == SDLK_o && ctrl) {
                        // TODO.
                    }

                    // Copy
                    if (key == SDLK_c && ctrl) {
                        // TODO.
                    }

                    // Full zoom out.
                    if (key == SDLK_f && ctrl) {
                        state.zoom_window.ZoomOutFull();
                    }

                    // Follow playback.
                    if (key == SDLK_f && !ctrl) {
                        follow_playback = !follow_playback;
                    }

                    // Zoom to selection.
                    if (key == SDLK_e && ctrl && state.Selection()) {
                        state.zoom_window.ZoomRange(state.Cursor(), *state.Selection());
                    }

                    // Zoom toggle one tracks.
                    if (key == SDLK_z && !shift) {
                        state.ToggleViewSingleTrack();
                    }

                    // Zoom toggle one channels on selected track.
                    if (key == SDLK_z && shift && !ctrl) {
                        state.ToggleViewSingleChannel(mouse_y);
                    }

                    // Zoom toggle one channels and one track on selected track.
                    if (key == SDLK_z && shift && ctrl) {
                        state.ToggleViewSingleChannel(mouse_y);
                        state.ToggleViewSingleTrack();
                    }

                    // Zoom in vertically.
                    if ((key == SDLK_PLUS || key == SDLK_KP_PLUS) && ctrl) {
                        state.zoom_window.ZoomInVertical();
                    }

                    // Zoom in horizontally.
                    if ((key == SDLK_PLUS || key == SDLK_KP_PLUS) && !ctrl) {
                        state.zoom_window.ZoomIn(.5f);
                    }

                    // Zoom out vertically.
                    if ((key == SDLK_MINUS || key == SDLK_KP_MINUS) && ctrl) {
                        state.zoom_window.ZoomOutVertical();
                    }

                    // Zoom out horizontally.
                    if ((key == SDLK_MINUS || key == SDLK_KP_MINUS) && !ctrl) {
                        state.zoom_window.ZoomOut(.5f);
                    }

                    // Reset vertical zoom.
                    if (key == SDLK_0 && ctrl) {
                        state.zoom_window.ZoomOutFullVertical();
                    }

                    // Scroll and move the cursor to the beginning.
                    if (key == SDLK_HOME) {
                        state.SetCursor(0.0f);
                        state.zoom_window.PanTo(0.0f);
                    }

                    // Scroll and move the cursor to the end.
                    if (key == SDLK_END) {
                        state.SetCursor(state.zoom_window.MaxX());
                        state.zoom_window.PanTo(
                            state.zoom_window.MaxX() -
                            (state.zoom_window.Right() - state.zoom_window.Left()));
                    }

                    // Pan left.
                    if (key == SDLK_LEFT) {
                        state.zoom_window.PanLeft();
                    }

                    // Pan right.
                    if (key == SDLK_RIGHT) {
                        state.zoom_window.PanRight();
                    }

                    // Move track up.
                    if (key == SDLK_UP && shift) {
                        state.MoveTrackUp();
                    }

                    // Scroll one channel up.
                    if (key == SDLK_UP && !shift && ctrl) {
                        state.ScrollChannelUp();
                    }

                    // Scroll one track up.
                    if (key == SDLK_UP && !shift && !ctrl) {
                        state.ScrollTrackUp();
                    }

                    // Move track down.
                    if (key == SDLK_DOWN && shift) {
                        state.MoveTrackDown();
                    }

                    // Scroll one channel down.
                    if (key == SDLK_DOWN && !shift && ctrl) {
                        state.ScrollChannelDown();
                    }

                    // Scroll one track down.
                    if (key == SDLK_DOWN && !shift && !ctrl) {
                        state.ScrollTrackDown();
                    }

                    // Save selection.
                    if (key == SDLK_s && ctrl) {
                        // TODO.
                    }

                    // Toggle spectrogram view.
                    if (key == SDLK_s) {
                        view_spectrogram = !view_spectrogram;
                    }

                    // Toggle bark scale spectrograms.
                    if (key == SDLK_b) {
                        view_bark_scale = !view_bark_scale;
                    }

                    // Toggle playback.
                    if (key == SDLK_SPACE) {
                        state.SetLooping(shift);
                        state.TogglePlayback();
                    }

                    // Close selected track.
                    if (key == SDLK_w && ctrl && !shift) {
                        state.UnloadSelectedTrack();
                    }

                    // Close all tracks.
                    if (key == SDLK_w && ctrl && shift) {
                        state.UnloadFiles();
                    }

                    // Reload all files.
                    if (key == SDLK_r && ctrl) {
                        state.ReloadFiles();
                    }

                    // Toggle auto-reload.
                    if (key == SDLK_r && !ctrl) {
                        // TODO.
                    }

                    // dB scale waveform.
                    if (key == SDLK_d) {
                        state.zoom_window.ToggleDbVerticalScale();
                    }

                    break;
                }
            }
        }

        float play_time;
        bool playing = state.Playing(&play_time);

        if (follow_playback) {
            if (play_time > state.zoom_window.Right() || play_time < state.zoom_window.Left()) {
                state.zoom_window.PanTo(play_time);
            }
        }

        scroll_value = state.zoom_window.Left();
        scroll_max =
            state.zoom_window.MaxX() - (state.zoom_window.Right() - state.zoom_window.Left());

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // static bool show_demo = true;
        // ImGui::ShowDemoWindow(&show_demo);

        ImGuiStyle& style = ImGui::GetStyle();
        status_bar_height = ImGui::GetFrameHeight() * 3.0f + 2.0f * style.WindowPadding.y;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        win_width = viewport->Size.x;
        ImGui::SetNextWindowPos(
            ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - status_bar_height));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, status_bar_height));

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("StatusBar", nullptr, flags)) {
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - 2.0f * style.WindowPadding.x);
            ImGui::SliderFloat("##Time", &scroll_value, 0.0f, scroll_max, "");
            state.zoom_window.PanTo(scroll_value);
            if (ImGui::BeginTable("status_table", 3, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Selection");
                ImGui::TableNextColumn();
                if (!state.Selection()) {
                    const float cursor_time = state.Cursor();
                    const float cursor_minutes = std::floor(cursor_time / 60.0f);
                    const float cursor_seconds = cursor_time - 60.0f * cursor_minutes;
                    ImGui::Text("%02.f:%06.03f", cursor_minutes, cursor_seconds);
                } else {
                    float selection_start_time = state.Cursor();
                    float selection_end_time = *state.Selection();
                    if (selection_start_time > selection_end_time)
                        std::swap(selection_start_time, selection_end_time);
                    const float selection_start_minutes = std::floor(selection_start_time / 60.0f);
                    const float selection_start_seconds =
                        selection_start_time - 60.0f * selection_start_minutes;
                    const float selection_end_minutes = std::floor(selection_end_time / 60.0f);
                    const float selection_end_seconds =
                        selection_end_time - 60.0f * selection_end_minutes;
                    const float selection_length_time = selection_end_time - selection_start_time;
                    const float selection_length_minutes =
                        std::floor(selection_length_time / 60.0f);
                    const float selection_length_seconds =
                        selection_length_time - 60.0f * selection_length_minutes;
                    const int samplerate = state.SelectedTrack() ? state.GetCurrentSamplerate() : 0;
                    const int samples = static_cast<int>(selection_length_time * samplerate);
                    const float frequency =
                        (selection_length_time > 1e-5f && selection_length_time < 10.0f)
                            ? 1.0f / selection_length_time
                            : 0.0f;
                    ImGui::Text(
                        "%02.f:%06.03f - %02.f:%06.03f  Length %02.f:%06.03f  %d samples  %.03f Hz",
                        selection_start_minutes, selection_start_seconds, selection_end_minutes,
                        selection_end_seconds, selection_length_minutes, selection_length_seconds,
                        samples, frequency);
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Pointer");
                ImGui::TableNextColumn();
                if (state.tracks.size()) {
                    const float pointer_time = state.zoom_window.GetTime(mouse_x);
                    const float pointer_minutes = std::floor(pointer_time / 60.0f);
                    const float pointer_seconds = pointer_time - 60.0f * pointer_minutes;

                    ZoomWindow& z = state.zoom_window;
                    int track_number = z.GetTrack(mouse_y);
                    if (track_number < static_cast<int>(state.tracks.size())) {
                        Track& t = state.GetTrack(track_number);
                        float display_gain_db = 20.0f * std::log10(z.VerticalZoom());
                        if (t.audio_buffer) {
                            // Track space: On what y-coordinate is the pointer?
                            float y =
                                1.f - std::fmod(z.Top() + (z.Bottom() - z.Top()) * mouse_y, 1.f);
                            // Channel space: On what y-coordinate is the pointer?
                            float num_channels =
                                t.selected_channel ? 1 : t.audio_buffer->NumChannels();
                            y = std::fmod(num_channels * y, 1.f);
                            if (view_spectrogram) {
                                float f = 0.f;
                                float nyquist_freq = 0.5f * t.audio_buffer->Samplerate();
                                if (view_bark_scale) {
                                    const float bark_scaling =
                                        26.81f * nyquist_freq / (1960.f + nyquist_freq) - 0.53f;
                                    f = 1960.f * (bark_scaling * y + 0.53f) /
                                        (26.28f - bark_scaling * y);
                                } else {
                                    f = y * nyquist_freq;
                                }
                                ImGui::Text("%02.f:%06.03f  Frequency %.0f Hz  Gain %.1f",
                                            pointer_minutes, pointer_seconds, std::round(f),
                                            display_gain_db);
                            } else {
                                float a = 0;
                                if (z.DbVerticalScale()) {
                                    a = -(1.f - 2.f * std::abs(y - 0.5f) / z.VerticalZoom()) * 60;
                                } else {
                                    a = 20.f *
                                        std::log10(2.f * std::abs(y - 0.5f) / z.VerticalZoom());
                                }
                                ImGui::Text("%02.f:%06.03f  Amplitude %.1f dBFS  Gain %.1f",
                                            pointer_minutes, pointer_seconds, a, display_gain_db);
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));

        ImGuiWindowFlags overlay_flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMouseInputs;
        ImGui::Begin("##Overlay", nullptr, overlay_flags);

        renderer->Draw(&state, static_cast<int>(io.DisplaySize.x),
                       static_cast<int>(io.DisplaySize.y), static_cast<int>(status_bar_height),
                       1.0f, view_spectrogram, view_bark_scale, playing, play_time,
                       [&style](float y, const char* text) {
                           ImGui::SetCursorPosY(y + style.WindowPadding.y);
                           ImGui::Text("%s", text);
                       });

        ImGui::End();  // End of overlay window.

        bool view_reset;
        state.CreateResources(&view_reset);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    delete renderer;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
