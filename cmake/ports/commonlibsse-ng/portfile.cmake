vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/CommonLibVR
    REF c5424463bba9af0d75cde8640ba7ddd4cacb9e39
    SHA512 7090a9f3c34930cd59d56dac6b35d86a6a74dad64161ad47f76556b0e49bfec8544f53e5d8abf70beee369ef8cfa70159823c1be42680cbb0958428c2074ed55
    HEAD_REF ng
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH2
    REPO ValveSoftware/openvr
    REF 60eb187801956ad277f1cae6680e3a410ee0873b
    SHA512 bb85b4705e7095ac65df9969112b2df8930cee7917cc5f14231c5a0ffeed7a73ffa60727fd32f8786a403656f95a3ec0f80bf3ceabc5b8ede964aefb920bc718
    HEAD_REF master
)

file(COPY "${SOURCE_PATH2}/" DESTINATION "${SOURCE_PATH}/extern/openvr")

vcpkg_check_features(OUT_FEATURE_OPTIONS RUNTIME_OPTIONS
    FEATURES se ENABLE_SKYRIM_SE ae ENABLE_SKYRIM_AE vr ENABLE_SKYRIM_VR)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${RUNTIME_OPTIONS}
        -DBUILD_TESTS=OFF 
        -DSKSE_SUPPORT_XBYAK=ON
        -DSKSE_SUPPORT_PATCH_SAFETY=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/CommonLibSSE")

file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${SOURCE_PATH2}/headers/openvr.h" DESTINATION ${CURRENT_PACKAGES_DIR}/include)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST
    "${SOURCE_PATH}/COPYING.txt"
    "${SOURCE_PATH}/EXCEPTIONS.md"
    "${SOURCE_PATH}/licenses/LICENSE-MIT.txt"
    "${SOURCE_PATH2}/LICENSE")
