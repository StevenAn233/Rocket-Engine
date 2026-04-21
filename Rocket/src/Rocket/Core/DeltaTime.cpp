module;
module DeltaTime;

import Instrumentor;
import String;

namespace rke
{
    static std::chrono::steady_clock::time_point last_time_{};
    static double delta_time_{};
    static double slow_fps_{};
    static double smoothed_fps_{};

    float  DeltaTime::get() { return static_cast<float>(delta_time_); }
    uint32 DeltaTime::get_fps() { return static_cast<uint32>(1.0f / delta_time_); }
    uint32 DeltaTime::get_slow_fps() { return static_cast<uint32>(slow_fps_); }
    uint32 DeltaTime::get_smoothed_fps() { return static_cast<uint32>(smoothed_fps_); }

    void DeltaTime::update()
    {
        RKE_PROFILE_FUNCTION();

        const auto current_time{ std::chrono::steady_clock::now() };
        const auto delta{ current_time - last_time_ };
        delta_time_ = delta / std::chrono::duration<double>(1.0);
        last_time_  = current_time;

        update_slow_fps();
        update_smoothed_fps();
    }

    void DeltaTime::update_smoothed_fps()
    {
        RKE_PROFILE_FUNCTION();

        static constexpr float ALPHA{ 5.0f };
        double current_fps{ 1.0 / delta_time_ };

        // use Lerp to smooth the value
        double lerp_alpha {
            glm::clamp(delta_time_ * ALPHA, 0.0, 1.0)
        };
        smoothed_fps_ = glm::mix(smoothed_fps_, current_fps, lerp_alpha);
    }

    void DeltaTime::update_slow_fps()
    {
        RKE_PROFILE_FUNCTION();

        static double time_accumulator{};
        static uint32 frame_counter{};

        time_accumulator += delta_time_;
        frame_counter++;

        if(time_accumulator >= 0.5f) // hard-coded
        {
            slow_fps_ = frame_counter / time_accumulator;
            time_accumulator = 0;
            frame_counter    = 0;
        }
    }
}
