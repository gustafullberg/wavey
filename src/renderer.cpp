#include "renderer.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <sstream>

#include "label_renderer.hpp"
#include "primitive_renderer.hpp"
#include "sample_line_shader.hpp"
#include "sample_point_shader.hpp"
#include "spectrogram_shader.hpp"
#include "state.hpp"
#include "wave_shader.hpp"

namespace {
std::string TimeToString(float t, float dt, bool show_minutes) {
    int decimals;
    if (dt < 0.01f) {
        decimals = 3;
    } else if (dt < 0.1f) {
        decimals = 2;
    } else if (dt < 1.f) {
        decimals = 1;
    } else {
        decimals = 0;
    }

    // Convert to minutes and seconds.
    t += 1e-4f;  // Avoid rounding errors.
    const int minutes = std::floor(t / 60.f);
    t -= 60.f * minutes;
    const float seconds = t;

    std::ostringstream str;
    if (show_minutes) {
        str << minutes << ":";
        str << std::setw(decimals ? 2 + 1 + decimals : 2) << std::setfill('0');
    }
    str.precision(decimals);
    str << std::fixed << seconds;

    return str.str();
}

static std::pair<float, int> available_resolutions[] = {
    {0.001f, 1}, {0.005f, 5}, {0.01f, 10}, {0.05f, 5}, {0.1f, 10},  {0.5f, 5},    {1.0f, 10},
    {5.0f, 5},   {10.0f, 10}, {30.0f, 3},  {60.0f, 2}, {300.0f, 5}, {600.0f, 10}, {1800.0f, 3}};

void TimelineResolution(float view_length, float& dt_labeled, int& unlabeled_ratio) {
    // Find optimal timeline resolution with at most kMaxNumLabels labeled markers.
    constexpr float kMaxNumLabels = 10.0f;
    constexpr int kNumResolutions =
        sizeof(available_resolutions) / sizeof(available_resolutions[0]);
    for (int i = 0; i < kNumResolutions; i++) {
        const auto& r = available_resolutions[i];
        if (view_length < r.first * kMaxNumLabels || i == kNumResolutions - 1) {
            dt_labeled = r.first;
            unlabeled_ratio = r.second;
            return;
        }
    }
}

float TimelineStart(float view_start, float dt) {
    return std::floor(view_start / dt) * dt;
}
}  // namespace

class RendererImpl : public Renderer {
   public:
    RendererImpl();
    virtual ~RendererImpl();
    virtual void Draw(State* state,
                      int win_width,
                      int win_height,
                      float scale_factor,
                      bool view_spectrogram,
                      bool view_bark_scale,
                      bool playing,
                      float play_time);
    virtual float TimelineHeight() { return timeline_height; }

   private:
    WaveShader wave_shader;
    SampleLineShader sample_line_shader;
    SamplePointShader sample_point_shader;
    SpectrogramShader spectrogram_shader;
    PrimitiveRenderer prim_renderer;
    LabelRenderer label_renderer;
    float timeline_height = 0;

    // Color palette.
    glm::vec4 color_background{0.2f, 0.2f, 0.2f, 1.0f};
    glm::vec4 color_timeline{0.7f, 0.7f, 0.7f, 0.5f};
    glm::vec4 color_selection{0.5f, 0.9f, 0.5f, 0.1f};
    glm::vec4 color_line{0.5f, 0.9f, 0.5f, 0.1f};
    glm::vec4 color_cursor{0.5f, 0.9f, 0.5f, 0.5f};
    glm::vec4 color_play_indicator{0.9f, 0.5f, 0.5f, 1.0f};
    glm::vec4 color_text{0.5f, 0.9f, 0.5f, 1.0f};
    glm::vec4 color_text_selected{0.9f, 0.9f, 0.5f, 1.0f};
    glm::vec4 color_text_shadow{0.1f, 0.2f, 0.1f, 1.f};
    glm::vec4 color_text_timeline{0.7f, 0.7f, 0.7f, 1.0f};
    glm::vec4 color_wave{0.5f, 0.9f, 0.5f, 1.0f};
    glm::vec4 color_sample_point{0.5f, 0.9f, 0.5f, 1.0f};
    glm::vec4 color_sample_line{0.5f, 0.9f, 0.5f, 0.3f};
};

RendererImpl::RendererImpl() {
    wave_shader.Init(color_wave);
    sample_line_shader.Init(color_sample_line);
    sample_point_shader.Init(color_sample_point);
    spectrogram_shader.Init();
    prim_renderer.Init();
    label_renderer.Init();

    glClearColor(color_background.r, color_background.g, color_background.b, color_background.a);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
}

RendererImpl::~RendererImpl() {
    wave_shader.Terminate();
    sample_line_shader.Terminate();
    sample_point_shader.Terminate();
    spectrogram_shader.Terminate();
    prim_renderer.Terminate();
    label_renderer.Terminate();
}

void RendererImpl::Draw(State* state,
                        int win_width,
                        int win_height,
                        float scale_factor,
                        bool view_spectrogram,
                        bool view_bark_scale,
                        bool playing,
                        float play_time) {
    const ZoomWindow& z = state->zoom_window;
    const float cursor_x = state->Cursor() - z.Left();
    const float view_length = z.Right() - z.Left();

    glViewport(0, 0, win_width, win_height);
    glClear(GL_COLOR_BUFFER_BIT);

    // Timeline
    float dt_labeled;
    int unlabeled_ratio;
    TimelineResolution(view_length, dt_labeled, unlabeled_ratio);
    timeline_height = 2.f * (state->max_timelabel_height * scale_factor);
    glViewport(0, win_height - timeline_height, win_width, timeline_height);
    const glm::mat4 mvp_timeline = glm::ortho(0.f, view_length, 0.f, 1.f, -1.f, 1.f);
    const float start = TimelineStart(z.Left(), dt_labeled) - z.Left();
    const float dt = dt_labeled / unlabeled_ratio;
    const bool show_minutes = z.Right() >= 60.f;
    for (int i = 0; start + i * dt < view_length; i++) {
        const float t_view = start + i * dt;
        const bool labeled_marker = i % unlabeled_ratio == 0;
        prim_renderer.DrawLine(mvp_timeline, glm::vec2(t_view, 0.0f),
                               glm::vec2(t_view, labeled_marker ? 0.5f : 0.25f), color_timeline);
        if (labeled_marker) {
            const float t = t_view + z.Left();
            if (t > 0.f) {
                std::string key = TimeToString(t, dt_labeled, show_minutes);
                if (state->HasTimeLabel(key)) {
                    const float x = t_view / view_length * win_width;
                    const GpuLabel& label = state->GetTimeLabel(key);
                    label_renderer.Draw(label, x, 0.f, win_width, timeline_height, scale_factor,
                                        color_text_timeline, color_text_shadow, true);
                }
            }
        }
    }

    const float view_height = win_height - timeline_height;
    glViewport(0, 0, win_width, view_height);
    glm::mat4 mvp = glm::ortho(0.f, view_length, z.Bottom(), z.Top(), -1.f, 1.f);

    for (int i = 0; i < static_cast<int>(state->tracks.size()); i++) {
        const Track& t = state->GetTrack(i);
        if (!t.audio_buffer) {
            continue;
        }

        if (state->SelectedTrack() && i == state->SelectedTrack()) {
            prim_renderer.DrawQuad(mvp, glm::vec2(0.f, i + 1), glm::vec2(view_length, i),
                                   color_selection);
        }

        const int num_channels = t.audio_buffer->NumChannels();
        const int samplerate = t.audio_buffer->Samplerate();
        const float length = t.audio_buffer->Duration();

        for (int c = 0; c < num_channels; c++) {
            const float trackOffset = i;
            const float channelOffset = (2.f * c + 1.f) / (2.f * num_channels);
            glm::mat4 mvp_channel =
                glm::translate(mvp, glm::vec3(0.f, trackOffset + channelOffset, 0.f));
            mvp_channel = glm::scale(mvp_channel, glm::vec3(1.f, -0.5f / num_channels, 1.f));

            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 1.f), glm::vec2(view_length, 1.f),
                                   color_line);
            prim_renderer.DrawLine(mvp_channel, glm::vec2(0.f, 0.f),
                                   glm::vec2(length - z.Left(), 0.f), color_line);
            if (view_spectrogram) {
                if (t.gpu_spectrogram) {
                    spectrogram_shader.Draw(mvp_channel, z.Left(), samplerate, view_bark_scale,
                                            z.VerticalZoom());
                    t.gpu_spectrogram->Draw(c);
                }
            } else {
                if (t.gpu_waveform) {
                    float samples_per_pixel = (z.Right() - z.Left()) * samplerate / win_width;
                    const bool draw_low_res = samples_per_pixel > 1000.f;
                    const bool draw_discrete_samples = samples_per_pixel < 0.25f;
                    const float rate = draw_low_res ? samplerate * 2.f / 1000.f : samplerate;
                    if (draw_discrete_samples) {
                        sample_line_shader.Draw(mvp_channel, rate, z.VerticalZoom());
                        t.gpu_waveform->DrawPoints(c, z.Left(), z.Right(), draw_low_res);
                        sample_point_shader.Draw(mvp_channel, rate, z.VerticalZoom());
                        t.gpu_waveform->DrawPoints(c, z.Left(), z.Right(), draw_low_res);
                    } else {
                        wave_shader.Draw(mvp_channel, rate, z.VerticalZoom());
                        t.gpu_waveform->DrawLines(c, z.Left(), z.Right(), draw_low_res);
                    }
                }
            }

            if (state->GetViewMode() == CHANNEL && t.gpu_channel_labels[c]) {
                float y =
                    std::round(view_height * (i + static_cast<float>(c) / num_channels - z.Top()) /
                               (z.Bottom() - z.Top()));
                label_renderer.Draw(*t.gpu_channel_labels[c], 0.f, y, win_width, view_height,
                                    scale_factor, color_text_selected, color_text_shadow, false);
            }
        }

        if (state->GetViewMode() != CHANNEL && t.gpu_track_label) {
            float y = std::round(view_height * (i - z.Top()) / (z.Bottom() - z.Top()));
            bool selected = state->SelectedTrack() && *state->SelectedTrack() == i;
            label_renderer.Draw(*t.gpu_track_label, 0.f, y, win_width, view_height, scale_factor,
                                selected ? color_text_selected : color_text, color_text_shadow,
                                false);
        }
    }

    // Selection.
    if (state->Selection()) {
        const float selection_x = *state->Selection() - z.Left();
        prim_renderer.DrawQuad(mvp, glm::vec2(cursor_x, z.Bottom()),
                               glm::vec2(selection_x, z.Top()), color_selection);
    }

    // Cursor.
    prim_renderer.DrawLine(mvp, glm::vec2(cursor_x, z.Bottom()), glm::vec2(cursor_x, z.Top()),
                           color_cursor);

    // Play position indicator.
    if (playing) {
        const float play_x = play_time - z.Left();
        prim_renderer.DrawLine(mvp, glm::vec2(play_x, state->last_played_track + 1),
                               glm::vec2(play_x, state->last_played_track), color_play_indicator);
    }
}

Renderer* Renderer::Create() {
    return new RendererImpl();
}

Renderer::~Renderer() {}
