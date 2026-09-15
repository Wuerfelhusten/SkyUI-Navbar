#include "PCH.h"

// vcpkg must build the library and consumer with the same runtime layouts.
// Preset names alone do not configure an already compiled dependency.
#ifdef ENABLE_SKYRIM_SE
static_assert(NAVBAR_EXPECT_SE, "CommonLib SE support does not match the selected preset");
#else
static_assert(!NAVBAR_EXPECT_SE, "CommonLib was built without requested SE support");
#endif
#ifdef ENABLE_SKYRIM_AE
static_assert(NAVBAR_EXPECT_AE, "CommonLib AE support does not match the selected preset");
#else
static_assert(!NAVBAR_EXPECT_AE, "CommonLib was built without requested AE support");
#endif
#ifdef ENABLE_SKYRIM_VR
static_assert(NAVBAR_EXPECT_VR, "CommonLib VR support does not match the selected preset");
#else
static_assert(!NAVBAR_EXPECT_VR, "CommonLib was built without requested VR support");
#endif
