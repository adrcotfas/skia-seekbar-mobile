#include "OverlayTypefaceProvider.h"

#include "include/core/SkTypeface.h"

#ifdef __ANDROID__
#include "include/core/SkFontMgr.h"
#include "include/ports/SkFontMgr_android.h"
#include "include/ports/SkFontMgr_android_ndk.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

sk_sp<SkTypeface> CreateDefaultOverlayTypeface() {
#ifdef __ANDROID__
    // Keep the font manager alive for the lifetime of the process.
    static sk_sp<SkFontMgr> mgr = []() -> sk_sp<SkFontMgr> {
        auto m = SkFontMgr_New_AndroidNDK(false, SkFontScanner_Make_FreeType());
        if (!m) {
            m = SkFontMgr_New_Android(nullptr, SkFontScanner_Make_FreeType());
        }
        return m;
    }();

    if (!mgr) return nullptr;

    auto face = mgr->matchFamilyStyle(nullptr, SkFontStyle());
    if (!face) {
        face = mgr->legacyMakeTypeface(nullptr, SkFontStyle());
    }
    return face;
#else
    // iOS/macOS should use SkFontMgr_New_CoreText(...) (Objective-C++ TU) when you wire it in.
    return nullptr;
#endif
}
