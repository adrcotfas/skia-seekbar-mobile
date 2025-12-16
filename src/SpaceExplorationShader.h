// src/SpaceExplorationShader.h
#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/effects/SkRuntimeEffect.h"

// Star Nest by Pablo Roman Andrioli (MIT License)
// Animated space exploration shader for video simulation
class SpaceExplorationShader {
public:
    bool initialize() {
        auto result = SkRuntimeEffect::MakeForShader(SkString(kShaderCode));
        if (!result.effect) return false;
        effect_ = result.effect;
        return true;
    }

    void setTime(float t) { time_ = t; }
    [[nodiscard]] float getTime() const { return time_; }
    [[nodiscard]] bool isReady() const { return effect_ != nullptr; }

    void render(SkCanvas* canvas, const SkRect& bounds) {
        if (!canvas || !effect_) return;

        SkRuntimeShaderBuilder builder(effect_);
        builder.uniform("iResolution") = SkV2{bounds.width(), bounds.height()};
        builder.uniform("iTime") = time_;

        auto shader = builder.makeShader();
        if (!shader) return;

        SkPaint paint;
        paint.setShader(shader);

        canvas->save();
        canvas->translate(bounds.left(), bounds.top());
        canvas->drawRect(SkRect::MakeWH(bounds.width(), bounds.height()), paint);
        canvas->restore();
    }

private:
    sk_sp<SkRuntimeEffect> effect_;
    float time_ = 0.0f;

    static constexpr const char* kShaderCode = R"(
uniform float2 iResolution;
uniform float iTime;

const int iterations = 17;
const float formuparam = 0.53;
const int volsteps = 20;
const float stepsize = 0.1;
const float zoom = 0.800;
const float tile = 0.850;
const float speed = 0.010;
const float brightness = 0.0015;
const float darkmatter = 0.300;
const float distfading = 0.730;
const float saturation = 0.850;

half4 main(vec2 fragCoord) {
    vec2 uv = fragCoord.xy / iResolution.xy - 0.5;
    uv.y *= iResolution.y / iResolution.x;
    vec3 dir = vec3(uv * zoom, 1.0);
    float time = iTime * speed + 0.25;

    float a1 = 0.5 + time * 0.1;
    float a2 = 0.8 + time * 0.05;
    mat2 rot1 = mat2(cos(a1), sin(a1), -sin(a1), cos(a1));
    mat2 rot2 = mat2(cos(a2), sin(a2), -sin(a2), cos(a2));
    dir.xz *= rot1;
    dir.xy *= rot2;
    vec3 from = vec3(1.0, 0.5, 0.5);
    from += vec3(time * 2.0, time, -2.0);
    from.xz *= rot1;
    from.xy *= rot2;

    float s = 0.1;
    float fade = 1.0;
    vec3 v = vec3(0.0);
    for (int r = 0; r < volsteps; r++) {
        vec3 p = from + s * dir * 0.5;
        p = abs(vec3(tile) - mod(p, vec3(tile * 2.0)));
        float pa = 0.0;
        float a = 0.0;
        for (int i = 0; i < iterations; i++) {
            p = abs(p) / dot(p, p) - formuparam;
            a += abs(length(p) - pa);
            pa = length(p);
        }
        float dm = max(0.0, darkmatter - a * a * 0.001);
        a *= a * a;
        if (r > 6) fade *= 1.0 - dm;
        v += fade;
        v += vec3(s, s * s, s * s * s * s) * a * brightness * fade;
        fade *= distfading;
        s += stepsize;
    }
    v = mix(vec3(length(v)), v, saturation);
    return half4(v * 0.01, 1.0);
}
)";
};
