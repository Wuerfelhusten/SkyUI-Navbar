// Execute the reference AS2 logic, not a reimplementation of FaderMenu.
// Native queuing, Scaleform callbacks and presentation still need in-game testing.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const js = fs.readFileSync(process.argv[2], 'utf8')
    .replace(/^\s*var \w+;\s*$/gm, '')
    .replace(/function FaderMenu\(\)/, 'constructor()')
    .replace(/^(\s*)function /gm, '$1');
let completions = 0;
const Fader = vm.runInNewContext(js + '\nFaderMenu;', {
    MovieClip: class { constructor() { this.mc_FadeRect = { _alpha: 0 }; } },
    Color: function () { this.setRGB = () => {}; },
    Shared: { GlobalFunc: { Lerp: (a, b, lo, hi, x) => a + (b - a) * (x - lo) / (hi - lo) } },
    gfx: { io: { GameDelegate: {
        addCallBack() {},
        call(name) { assert.equal(name, 'FadeDone'); completions++; }
    } } }, Math
});
for (const [duration, minimum] of [[0.01, 0], [0.5, 0.2], [1, 2], [60, 60]]) {
    for (const alpha of [0, 25, 100]) {
        const fader = new Fader();
        fader.FadeRect._alpha = alpha;
        fader.initFade(true, true, duration, minimum);
        const before = completions;
        fader.updateFade(duration + minimum + 1);
        assert.equal(fader.FadeRect._alpha, 0, 'instant step reaches transparent endpoint');
        assert.equal(fader.fFadeElapsedSecs, duration, 'native interpolation is clamped');
        assert.equal(completions, before + 1, 'normal FadeDone callback runs');
    }
}
const normal = new Fader();
// Skills entry needs an opaque native background, not the Map transparent endpoint.
for (const alpha of [0, 25, 100]) {
    const skills = new Fader();
    skills.FadeRect._alpha = alpha;
    skills.initFade(false, true, 0.001, 0); // Native helper inverts its fade-out argument.
    const before = completions;
    skills.updateFade(1 / 240);
    assert.equal(skills.FadeRect._alpha, 100, 'Skills background reaches black behind the 3D scene');
    assert.equal(completions, before + 1, 'native FadeDone still runs');
    skills.initFade(true, true, 1, 0); // Native StatsMenu Hide reverses the fade.
    skills.updateFade(1);
    assert.equal(skills.FadeRect._alpha, 0, 'Skills exit releases the background normally');
}
normal.initFade(true, true, 1, 0);
const before = completions;
normal.updateFade(0.25);
assert.equal(normal.FadeRect._alpha, 75, 'unaccelerated native fade still interpolates');
assert.equal(completions, before, 'normal fade does not complete prematurely');
const zero = new Fader();
zero.initFade(true, true, 0, 0);
assert.equal(zero.FadeRect._alpha, 100, 'zero-duration init alone would leave black');
console.log('Reference FaderMenu endpoint/callback checks passed');
