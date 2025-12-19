#include "OverlayTypefaceProvider.h"

#include "include/core/SkTypeface.h"
#include "include/core/SkFontMgr.h"

#ifdef __ANDROID__
#include "include/ports/SkFontMgr_android.h"
#include "include/ports/SkFontMgr_android_ndk.h"
#include "include/ports/SkFontScanner_FreeType.h"
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include "include/ports/SkFontMgr_mac_ct.h"
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
#elif defined(__APPLE__)
    // Use CoreText font manager for iOS/macOS
    static sk_sp<SkFontMgr> mgr = SkFontMgr_New_CoreText(nullptr);
    
    if (!mgr) return nullptr;

    auto face = mgr->matchFamilyStyle("SF Pro", SkFontStyle());
    if (!face) {
        // Fall back to Helvetica if SF Pro isn't available
        face = mgr->matchFamilyStyle("Helvetica Neue", SkFontStyle());
    }
    if (!face) {
        // Last resort: system default
        face = mgr->matchFamilyStyle(nullptr, SkFontStyle());
    }
    return face;
#else
    return nullptr;
#endif
}
