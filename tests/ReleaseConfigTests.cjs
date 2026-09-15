const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = process.argv[2];
const json = name => JSON.parse(fs.readFileSync(path.join(root, name), 'utf8'));
const manifest = json('vcpkg.json');
const port = json('cmake/ports/commonlibsse-ng/vcpkg.json');
const presets = json('CMakePresets.json');
const cmake = fs.readFileSync(path.join(root, 'CMakeLists.txt'), 'utf8');
const version = cmake.match(/project\(SkyUINavbar VERSION ([0-9.]+)/)[1];
assert.equal(manifest['version-string'], version, 'project and dependency manifest version agree');
assert.deepEqual([...manifest['default-features']].sort(), ['ae', 'se'], 'default dependency build is FLATRIM');
const core = manifest.dependencies.find(d => d.name === 'commonlibsse-ng');
assert.equal(core['default-features'], false, 'do not inherit upstream VR default');
for (const preset of presets.configurePresets.filter(p => p.cacheVariables?.ENABLE_SKYRIM_SE)) {
    const cache = preset.cacheVariables;
    const expected = ['SE', 'AE', 'VR'].filter(r => cache['ENABLE_SKYRIM_' + r] === 'ON').map(r => r.toLowerCase());
    assert.equal(cache.VCPKG_MANIFEST_NO_DEFAULT_FEATURES, 'ON', preset.name);
    assert.deepEqual(cache.VCPKG_MANIFEST_FEATURES.split(';').sort(), expected.sort(), preset.name + ' forwards runtime selection');
    for (const runtime of expected) {
        assert.ok(port.features[runtime], 'overlay declares runtime feature');
        const dependency = manifest.features[runtime].dependencies.find(d => d.name === 'commonlibsse-ng');
        assert.equal(dependency['default-features'], false);
        assert.deepEqual(dependency.features, [runtime]);
    }
}
const flat = presets.configurePresets.find(p => p.name === 'FLATRIM').cacheVariables;
assert.equal(flat.ENABLE_SKYRIM_SE, 'ON');
assert.equal(flat.ENABLE_SKYRIM_AE, 'ON');
assert.equal(flat.ENABLE_SKYRIM_VR, 'OFF');
assert.equal(presets.buildPresets.find(p => p.name === 'Flatrim-Release').configurePreset, 'FLATRIM');
assert.equal(presets.configurePresets.find(p => p.name === 'common').cacheVariables.NAVBAR_DEPLOY_DIR, '${sourceDir}/dist');
console.log('Release version, portable deployment and SE/AE/VR dependency feature contracts verified.');
