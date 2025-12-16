# Skia seek bar demo
 
A small C++ app that renders a YouTube-like seek bar with **Skia**. The “video” surface is just a rendered background (not real media playback). The UI supports chapters (segments), an indefinite loading animation, drag-to-seek, and double-tap seeking (+/- 10s).

## Setup

### 1) Repo + dependencies

Prereqs: `git`, `python3`, `cmake`.

```bash
git clone <repo-url>
cd <repo>
./scripts/setup.sh
```

### 2) Platform build

**Android**
- Install Android SDK + NDK.
- Build Skia:

```bash
./scripts/build-skia-android.sh
```

- Run:

```bash
cd android
./gradlew :app:installDebug
```

**iOS (planned)**

- `ios/` is a placeholder for an upcoming wrapper project.

## Demo

Portrait:
 
<video controls playsinline muted loop width="360">
  <source src="assets/android_portrait.webm" type="video/webm" />
</video>

Landscape:
 
<video controls playsinline muted loop width="720">
  <source src="assets/android_landscape.webm" type="video/webm" />
</video>

Note: the demo background is not a real decoded video; it’s just a rendered surface so the UI interactions are easy to see.

## Project structure

- `src/` — native app entry point. Creates an SDL window + GLES context, wires Skia to it, forwards input events, and drives the UI library.
- `libs/skplayer_ui/` — reusable Skia UI library (seek bar + overlays). No platform/windowing code.
- `android/` — Android Gradle project. Uses `externalNativeBuild` to build the C++ code via the top-level `CMakeLists.txt`. Reuses SDL’s Android Java glue code.
- `ios/` — placeholder for an iOS wrapper (not wired up).
- `scripts/` — helper scripts (sync Skia deps, build Skia for Android).
- `third_party/` — external dependencies: Skia, SDL3, and `depot_tools`.
- `assets/` — demo media for the README.

## What to look at (quick tour)

- `src/main.cpp`: SDL3 window + GL setup, Skia surface creation, and input/event wiring.
- `libs/skplayer_ui/include/skplayer_ui/VideoContainer.h`: public UI API (`onPointer*`, `update`, `render`).
- `libs/skplayer_ui/src/VideoContainer.cpp`: UI state machine (play/pause, seek sessions, auto-hide).
- `libs/skplayer_ui/src/SeekBar.h`: segmented seek bar rendering + drag logic.
- `libs/skplayer_ui/src/SeekFeedbackOverlay.h`, `SeekChevronOverlay.h`, `SeekPreviewTooltip.h`: overlays.

## Design notes (brief)

- **Rendering:** Skia on top of an SDL3 + OpenGL ES backing surface.
- **UI API:** “game-loop style” usage: `onPointer*` → `update(dt, nowMs)` → `render(canvas)`.
- **Separation:** `libs/skplayer_ui` has no platform/windowing code; the app layer owns that.
- **Integration point:** `VideoContainer::Listener` reports `onPlay/onPause/onSeekTo` for wiring to a real player.
- **Typeface:** optional and non-owning at the config boundary; internal code uses `sk_sp`.

## `libs/skplayer_ui` API overview

The library is centered around `skplayer_ui::VideoContainer`: you feed it pointer events and a clock, and it draws into a `SkCanvas`.

```cpp
skplayer_ui::VideoContainer::Config cfg;
cfg.durationSeconds = 245.0f;
cfg.chapters = {{0.0f, "Intro"}, {42.0f, "Demo"}};
cfg.dpiScale = dpiScale;
cfg.overlayTypeface = typeface; // optional; non-owning

struct AppListener final : skplayer_ui::VideoContainer::Listener {
  void onPlay() override { /* start playback */ }
  void onPause() override { /* pause playback */ }
  void onSeekTo(float seconds) override { /* seek player */ }
} listener;

skplayer_ui::VideoContainer ui(cfg, &listener);

ui.setViewport(widthPx, heightPx);
ui.setLayout(/*portrait=*/true, /*videoCenterY=*/videoCenterY);

// input:
ui.onPointerDown(x, y, nowMs);
ui.onPointerMove(x, y);
ui.onPointerUp(x, y);

// frame:
ui.update(dtSeconds, nowMs);
ui.render(canvas);
```

Notes:
- `Config` is the “static” setup (duration, chapters, DPI scale, optional typeface).
- `Listener` is the integration point with a real player.
- `ThemeConstants.h` exposes a small set of DP constants shared between the app and the UI library.
- `nowMs` is used for gesture timing (double-tap / seek bursts). Coordinates are in pixels.

Includes:
- Segmented seek bar (chapters) + loading animation
- Play/pause button (morph + ripple)
- Seek preview tooltip (time + chapter title)
- Double-tap seek feedback (chevrons + +/- seconds)
- Time badge (`current / duration`)

Implementation details are in `libs/skplayer_ui/src/`.

## Known limitations / next steps

- No real media playback yet (seek position is simulated); the UI is ready to be driven by a player clock.
- iOS wrapper is not wired up yet (`ios/` is a placeholder).
- Add a small test harness for hit-testing and time formatting; add visual regression screenshots for the seek bar.
