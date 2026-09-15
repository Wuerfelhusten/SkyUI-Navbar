// Logic-only AS2 harness. It does not emulate Scaleform rendering or its font metrics.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const source = fs.readFileSync(process.argv[2], 'utf8');
const js = source
    .replace(/^    var \w+;\s*$/gm, '')
    .replace(/function NavbarWidget\(\)/, 'constructor()')
    .replace(/^    function /gm, '    ');
let time = 0;
let automaticTranslations = {};
class MovieClip {
    constructor() { this._x = this._y = 0; this._visible = true; this.points = []; }
    createEmptyMovieClip(name) {
        const clip = new MovieClip();
        clip._parent = this;
        return this[name] = clip;
    }
    attachMovie(symbol, name) {
        const sizes = {
            NavbarTabBackground: [48, 44],
            NavbarInventoryIcon: [120.5, 128.75], NavbarMapIcon: [113.5, 117.85],
            NavbarSkillsIcon: [98.8, 109.25], NavbarMagicIcon: [106.35, 116.9], ButtonArt: [28, 24],
            NavbarBestiaryIcon: [510.95, 381.8], NavbarAchievementsIcon: [414.6, 393.9],
            NavbarWaitIcon: [316, 429], NavbarCustomSkillsIcon: [116.5, 118.7], NavbarCharacterIcon: [798.65, 885.95]
        };
        assert.ok(sizes[symbol]);
        const clip = this.createEmptyMovieClip(name);
        clip.symbol = symbol;
        [clip._width, clip._height] = sizes[symbol];
        clip.getBounds = () => ({ xMin: 0, yMin: 0, xMax: sizes[symbol][0], yMax: sizes[symbol][1] });
        return clip;
    }
    createTextField(name, depth, x, y, width, height) {
        this[name] = {
            _text: '', _width: width, _height: height, _x: x, _y: y,
            get text() { return this._text; },
            set text(value) { this._text = this.noTranslate ? value : (automaticTranslations[value] || value); },
            setTextFormat(format) { this._width = this.text.length * 10 + 4; }
        };
    }
    globalToLocal(point) {}
    gotoAndStop(frame) { this.frame = frame; }
    getNextHighestDepth() { return this.nextDepth = (this.nextDepth || 0) + 1; }
    removeMovieClip() { this.removed = true; this._visible = false; }
    clear() { this.points = []; }
    beginFill(color, alpha) { this.fill = [color, alpha]; }
    moveTo(x, y) { this.points.push([x, y]); }
    lineTo(x, y) { this.points.push([x, y]); }
    endFill() {}
}
const flashGlobals = {};
const bindingRequests = [];
const Widget = vm.runInNewContext(js + '\nNavbarWidget;', {
    MovieClip, TextFormat: function () {}, Color: function (clip) { this.setRGB = color => { clip.tint = color; }; },
    getTimer: () => time, Math, isNaN, isFinite, Array, _global: flashGlobals,
    flash: { filters: { DropShadowFilter: function (...args) { this.args = args; } } },
    LoadVars: function () { this.load = path => bindingRequests.push({ path, loader: this }); }
});
const coverWidget = new Widget();
coverWidget.setTransitionCover(50, -320, -40, 1600, 760);
assert.equal(coverWidget.initialized, false, 'overlay does not initialize navigation tabs');
assert.deepEqual(coverWidget.transitionCover.fill, [0, 100]);
assert.equal(coverWidget.transitionCover._alpha, 50);
assert.deepEqual(coverWidget.transitionCover.points, [[-321, -41], [1601, -41], [1601, 761], [-321, 761], [-321, -41]]);
const cover = coverWidget.transitionCover;
coverWidget.setTransitionCover(120, 0, 0, 1280, 720);
assert.equal(coverWidget.transitionCover, cover, 'overlay shape is reused');
assert.equal(cover._alpha, 100);
assert.equal(cover.points.length, 5, 'redraw clears old viewport geometry');
coverWidget.setTransitionCover(-1, 0, 0, 1280, 720);
assert.equal(cover._alpha, 0);
const entries = [
    { menu: 'InventoryMenu', label: 'Inventory', available: true },
    { menu: 'ModMenu', label: 'Modded', available: true },
    { menu: 'MissingMenu', label: 'Missing', available: false }
];
let switches = [];
const widget = new Widget();
widget._parent = new MovieClip();
const api = { Translate(key) { return key === '$SkyUINavbar_Hold' ? '(hold)' : key; }, Ready() {}, SwitchTo(menu) { switches.push(menu); return true; } };
assert.equal(widget.initialize(api, 'InventoryMenu', entries,
    { left: 0, top: 0, right: 1280, bottom: 720 }), 3);
assert.equal(widget._x, 1280);
assert.equal(widget.totalHeight, 144);
assert.equal(widget._yscale, 80, 'config 100% preserves the former 80% visible size');
assert.equal(widget._y, (720 - 144 * 0.8) / 2);
widget.tabs.forEach((tab, i) => {
    assert.equal(tab._y, i * 50);
    assert.equal(tab.visual._x, -48, 'collapsed tab exposes only its icon');
    assert.ok(Math.abs(Math.max(tab.visual.icon._width, tab.visual.icon._height) - 21) < 1e-6,
        'icon fits the smaller 21px box without distorting its aspect ratio');
    assert.equal(tab.visual.icon._x + tab.visual.icon._width / 2, 24, 'icon stays horizontally centered');
    assert.equal(tab.visual.icon._y + tab.visual.icon._height / 2, 22, 'icon stays vertically centered');
    assert.equal(tab.visual.label._visible, false);
    assert.equal(tab.visual.background._width, 55, 'collapsed background includes offscreen frame overdraw');
    assert.equal(tab.visual.background._height, 44);
    assert.equal(tab.visual.background._x, 0);
    assert.equal(tab.visual.background._y, 0);
    assert.ok(!tab.visual.background._rotation, 'background is no longer rotated/distorted');
});
assert.equal(widget.tabs[0].visual.label.textColor, 0xFFFFFF);
assert.equal(widget.tabs[2].visual.label.textColor, 0xAAAAAA);
assert.equal(widget.tabs[0].visual.icon.symbol, 'NavbarInventoryIcon');
assert.equal(widget.tabs[1].visual.icon.symbol, 'NavbarMapIcon', 'unknown menus reuse the existing map icon');
widget.tabs[0].onRollOver();
assert.equal(widget.tabs[0].visual.icon.tint, 0xFFFFFF, 'selected stays white while hovered');
widget.tabs[0].onRollOut();
widget.tabs[1].onRollOver();
assert.equal(widget.tabs[1].visual.label.textColor, 0xEEEEEE);
assert.equal(widget.tabs[1].visual.icon.tint, 0xEEEEEE);
widget.tabs[1].onRollOut();
assert.equal(widget.tabs[1].visual.label.textColor, 0xAAAAAA);
assert.equal(widget.tabs[1].visual.icon.tint, 0xAAAAAA);
const iconsWidget = new Widget();
iconsWidget._parent = new MovieClip();
iconsWidget.initialize(api, 'MapMenu', ['MagicMenu', 'StatsMenu', 'MapMenu', 'InventoryMenu'].map(menu =>
    ({ menu, label: menu, available: true })), { left: 0, top: 0, right: 1280, bottom: 720 });
['Magic', 'Skills', 'Map', 'Inventory'].forEach((name, i) => {
    const icon = iconsWidget.tabs[i].visual.icon;
    assert.equal(icon.symbol, `Navbar${name}Icon`, 'menu identity, not order, determines icon');
    const bounds = icon.getBounds();
    assert.ok(Math.abs(icon._width / icon._height - bounds.xMax / bounds.yMax) < 1e-6);
});
widget.tabs.forEach(tab => { tab.hitTest = () => false; });
assert.equal(widget.containsPoint(1, 2), false, 'outside all tab shapes passes through');
widget.tabs[1].hitTest = (x, y, shape) => x === 17 && y === 9 && shape;
assert.equal(widget.containsPoint(17, 9), true, 'native hit testing uses exact tab shapes');
assert.equal(widget.containsPoint(18, 9), false, 'native hit testing does not capture outside points');
widget._visible = false;
assert.equal(widget.containsPoint(17, 9), false, 'hidden widget never captures native input');
widget._visible = true;
widget.nativeButton(1, true, 17, 9);
widget.nativeButton(1, false, 17, 9);
widget.nativeButton(2, true, 17, 9);
widget.nativeButton(2, false, 17, 9);
widget.nativeButton(8, true, 17, 9);
assert.equal(switches.length, 0, 'right, middle and wheel never navigate');
widget.nativeButton(0, false, 17, 9);
assert.equal(switches.length, 0, 'release without navbar press cannot navigate');
widget.nativeButton(0, true, 17, 9);
widget.nativeButton(0, false, 18, 9);
assert.equal(switches.length, 0, 'drag release outside cancels');
widget.nativeButton(0, true, 18, 9);
widget.nativeButton(0, false, 17, 9);
assert.equal(switches.length, 0, 'press outside and release inside cannot navigate');
widget.nativeButton(0, true, 17, 9);
widget.nativeButton(0, false, 17, 9);
assert.equal(switches.length, 1, 'exclusive native left click activates matching tab');
assert.equal(widget.pressedTab, null, 'release clears press ownership');
widget.transitioning = false;
switches = [];
widget.tabs[0].onRelease();
widget.tabs[2].onRelease();
assert.equal(switches.length, 0, 'active and unavailable tabs cannot navigate');
widget.tabs[1].onRelease();
widget.tabs[1].onRelease();
assert.equal(switches.length, 1, 'transition lock prevents duplicate requests');
time = 6101;
widget.onEnterFrame();
assert.equal(widget.transitioning, false);
widget.tabs[1].onRollOver();
for (let i = 0; i < 5; ++i) { time += 100; widget.onEnterFrame(); }
assert.equal(widget.tabs[1].progress, 22);
assert.equal(widget.tabs[1].visual._x, -widget.tabs[1].tabWidth, 'expanded row remains flush right');
assert.equal(widget.tabs[1].visual.background._width, widget.tabs[1].tabWidth + 7);
assert.equal(widget.tabs[1].visual.background._height, 44, 'expansion does not stretch the height');
assert.equal(widget.tabs[1].visual.label._alpha, 100);
assert.equal(widget.tabs[1].visual.label._visible, true);
widget.tabs[1].onRollOut();
for (let i = 0; i < 6; ++i) { time += 100; widget.onEnterFrame(); }
assert.equal(widget.tabs[1].progress, 0);
assert.equal(widget.tabs[1].visual.background._width, 55, 'background retracts with its offscreen overdraw');
assert.equal(widget.tabs[1].visual.label._visible, false);
widget.layout(-320, -40, 1600, 760);
assert.equal(widget._x, 1600);
assert.equal(widget._y, (-40 + 760 - widget.totalHeight * 0.8) / 2);
widget.layout(0, 0, 100, 720);
assert.equal(widget._x, 100, 'narrow viewport retains right anchor');
assert.ok(Math.abs(widget._x - widget.totalWidth * widget._xscale / 100) < 0.00001, 'expanded tabs fit visible width');
assert.ok(widget._xscale < 100);
const priorX = widget._x;
widget.layout(0, 0, NaN, 720);
assert.equal(widget._x, priorX, 'invalid viewport does not corrupt geometry');
widget.layout(0, 0, Infinity, 720);
assert.equal(widget._x, priorX, 'infinite viewport does not corrupt geometry');
widget.layout(0, 0, 1280, 72);
assert.equal(widget._yscale, 28.125, 'long rails including hints fit short viewports');
assert.ok(Math.abs(widget._y - 15.75) < 1e-6, 'scaled rows stay centered with room for hints');
widget._parent.globalToLocal = point => { point.x = (point.x - 20) / 2; point.y = (point.y - 10) / 2; };
widget.layout(20, 10, 2580, 1450);
assert.equal(widget._x, 1280, 'host transform respected');
assert.equal(widget._y, (720 - widget.totalHeight * 0.8) / 2);
const empty = new Widget();
empty._parent = new MovieClip();
assert.equal(empty.initialize(api, 'InventoryMenu', [], { left: 0, top: 0, right: 1280, bottom: 720 }), 0);
assert.equal(empty._visible, false);
assert.equal(empty.containsPoint(0, 0), false);
assert.equal(new Widget().containsPoint(0, 0), false, 'uninitialized widget never captures input');
console.log('Widget logic passed: dynamic tabs, availability, transitions, hover, right anchor, scaling, empty list.');

const from = new Widget();
from._parent = new MovieClip();
const bounds = { left: 0, top: 0, right: 1280, bottom: 720 };
from.initialize(api, 'InventoryMenu', entries, bounds);
from.tabs[0].progress = 8;
from.tabs[1].onRollOver();
time += 100;
from.onEnterFrame();
const partialY = from.tabs[1].progress;
assert.ok(partialY > 0 && partialY < 22);
assert.equal(from.tabs[1].visual._x + from.tabs[1].visual.background._width - 5, 2, 'open stroke ends stay two units beyond the viewport');
from.tabs[1].onRelease();
const state = from.captureAnimationState();
time += 100;
from.onEnterFrame();
from.tabs[1].onRollOut();
assert.equal(from.tabs[1].progress, partialY, 'outgoing tab freezes at captured position');
assert.equal(from.tabs[1].isHover, true, 'outgoing teardown cannot clear captured hover');
assert.equal(from.tabs[0].progress, state[0].position, 'outgoing rollout animation freezes too');
const restoredEntries = [entries[1], entries[0], entries[2]].map(entry => {
    const animation = state.find(s => s.menu === entry.menu);
    return { ...entry, animationY: animation.position, hoverTarget: animation.target, isHover: animation.hover };
});
const to = new Widget();
to._parent = new MovieClip();
time += 350;  // The target movie has a different initialization time.
to.initialize(api, 'ModMenu', restoredEntries, bounds);
assert.equal(to.tabs[0].progress, partialY, 'restore exact intermediate position by menu name');
assert.equal(to.tabs[0].visual._x, from.tabs[1].visual._x, 'restore the exact horizontal pixel position');
assert.equal(to.tabs[0].hoverTarget, 22);
assert.equal(to.tabs[0].isHover, true);
assert.equal(to.tabs[0].visual.label.textColor, 0xFFFFFF, 'new active color is not inherited');
assert.equal(to.tabs[1].progress, state[0].position, 'restore other tabs mid-rollout');
to.tabs.forEach(tab => { tab.hitTest = () => true; });
to.reconcileRestoredHover(1200, 20);
assert.equal(to.tabs[0].hoverTarget, 22, 'stationary mouse keeps inherited hover');
time += 50;
to.onEnterFrame();
assert.ok(Math.abs(to.tabs[0].progress - partialY - 5.5) < 0.00001, 'continue without catch-up jump');
to.tabs[0].hitTest = () => false;
to.reconcileRestoredHover(0, 300);
assert.equal(to.tabs[0].hoverTarget, 0, 'mouse exit clears inherited hover without prior rollover');
const beforeRollout = to.tabs[0].progress;
time += 50;
to.onEnterFrame();
assert.ok(to.tabs[0].progress < beforeRollout && to.tabs[0].progress > 0, 'exit animates smoothly, not a snap');
const settled = new Widget();
settled._parent = new MovieClip();
settled.initialize(api, 'ModMenu', [{ ...entries[1], animationY: 22, hoverTarget: 22, isHover: true }], bounds);
time += 100;
settled.onEnterFrame();
assert.equal(settled.tabs[0].progress, 22, 'fully extended tab remains fully extended');
const ordinaryOpen = new Widget();
ordinaryOpen._parent = new MovieClip();
ordinaryOpen.initialize(api, 'ModMenu', entries, bounds);
assert.equal(ordinaryOpen.tabs[1].progress, 0, 'normal opening without handoff starts clean');
console.log('Animation continuity passed: partial/settled hover, rollout, freeze, new active color, pointer exit, clean reopen.');
ordinaryOpen.tabs[2].onRollOver();
time += 100;
ordinaryOpen.onEnterFrame();
assert.ok(ordinaryOpen.tabs[2].progress > 0, 'unavailable entries still reveal their labels');
assert.equal(ordinaryOpen.tabs[2].visual.label.textColor, 0xEEEEEE, 'unavailable hovered tab uses the same grayscale palette');
for (const tab of ordinaryOpen.tabs) {
    for (const progress of [0, 1, 5.5, 11, 16.5, 21, 22]) {
        tab.progress = progress;
        tab.redraw();
        assert.ok(Math.abs(tab.visual._x + tab.visual.background._width - 5 - 2) < 0.000001,
            'frame opening remains offscreen at every hover position and label width');
        assert.ok(Math.abs(Math.max(tab.visual.icon._width, tab.visual.icon._height) - 21) < 1e-6,
            'hover never stretches the smaller icon');
    }
}

// Data-driven host hint adapters. Mock the documented SkyUI ButtonPanel contract.
const uiConfig = new Widget().navigationDefaults();
function makePanel(capacity = 4) {
    const panel = new MovieClip();
    panel.buttons = [];
    panel._buttonCount = 0;
    panel.buttonRenderer = 'MappedButton';
    panel.originalClicks = 0;
    panel.attachMovie = function () {
        const button = new MovieClip();
        button._parent = this;
        button.label = '';
        button._keyCodes = [];
        button.setButtonData = function (data) { this.label = data.text; this._keyCodes = [data.controls.keyCode]; };
        button.setPlatform = function (platform) { this.platform = platform; };
        button.onRelease = () => panel.originalClicks++;
        button.hitTest = (x, y) => x === 111 && y === 222;
        return button;
    };
    for (let i = 0; i < capacity; ++i) panel.buttons.push(panel.attachMovie());
    panel.addButton = function (data) {
        if (this._buttonCount >= this.buttons.length) return undefined;
        const button = this.buttons[this._buttonCount++];
        button.setButtonData(data);
        button._visible = true;
        return button;
    };
    panel.clearButtons = function () { this._buttonCount = 0; this.buttons.forEach(b => { b.label = ''; b._visible = false; }); };
    panel.doUpdateButtons = function () { this.visibleLabels = this.buttons.filter(b => b._visible && b.label).map(b => b.label); };
    panel.hideButtons = function () { this.buttons.forEach(b => b._visible = false); };
    panel.showButtons = function () { this.buttons.forEach(b => b._visible = b.label.length > 0); };
    return panel;
}
let cycleCount = 0;
let remembered = [56, 271];
let priority = 0;
const navigationAPI = { ...api, NextMenu() { ++cycleCount; return true; },
    SetCycleKeys(pc, pad, fallback) {
        const level = fallback ? 1 : 2;
        if (!Number.isInteger(pc) || !Number.isInteger(pad) || pc < 1 || pc > 263 || pc === 255 || pad < 266 || pad > 281 || level < priority) return false;
        remembered = [pc, pad]; priority = level; return true;
    }
};
function navigationWidget(menu, root, config = uiConfig) {
    const result = new Widget();
    result._parent = new MovieClip();
    result.initialize(navigationAPI, menu, entries, bounds);
    result.tabs.forEach(tab => tab.hitTest = () => false);
    result.configureNavigation(config, root);
    result.updateNavigation(...remembered, false);
    return result;
}
for (const [menu, label, method, key] of [
    ['InventoryMenu', '$Magic', 'openMagicMenu', 56],
    ['MagicMenu', '$Inventory', 'openInventoryMenu', 271]
]) {
    const panel = makePanel(2);
    const oldAdd = panel.addButton, oldClear = panel.clearButtons, oldLayout = panel.doUpdateButtons;
    const legacy = panel.addButton({ text: label, controls: { keyCode: key } });
    const favorite = panel.addButton({ text: '$Favorite', controls: { keyCode: 33 } });
    const oldClick = legacy.onRelease;
    const root = { Menu_mc: { navPanel: panel, inventoryLists: {}, itemCard: {}, [method]() {},
        _switchControls: { keyCode: key },
        _config: { Input: { controls: { pc: { switchTab: 56 }, gamepad: { switchTab: 271 } } } } } };
    const nav = navigationWidget(menu, root);
    assert.deepEqual(panel.visibleLabels, ['$Favorite']);
    assert.equal(legacy._visible, false);
    assert.equal(favorite.label, '$Favorite');
    for (let i = 0; i < 20; ++i) nav.updateNavigation(56, 271, i % 2 === 0);
    assert.equal(panel.buttons.length, 2, 'never allocate a custom button');
    assert.equal(panel._buttonCount, 2);
    assert.equal(panel.addButton, oldAdd);
    assert.equal(legacy.onRelease, oldClick, 'never replace a host click handler');
    assert.equal(nav.hintButton, undefined);
    assert.equal(nav.containsPoint(111, 222), false);
    panel.hideButtons();
    nav.updateNavigation(56, 271, false);
    panel.showButtons();
    assert.equal(legacy._visible, false);
    for (const [text, code] of [[label, 99], ['$Unrelated', key]]) {
        panel.clearButtons();
        panel.addButton({ text, controls: { keyCode: code } });
        panel.doUpdateButtons();
        assert.deepEqual(panel.visibleLabels, [text], 'both exact label and key must match');
    }
    panel.clearButtons();
    panel.addButton({ text: label, controls: { keyCode: key } });
    panel.doUpdateButtons();
    nav.onUnload();
    assert.equal(panel.clearButtons, oldClear);
    assert.equal(panel.doUpdateButtons, oldLayout);
    assert.deepEqual(panel.visibleLabels, [label], 'safe restoration on unload');
    for (let i = 0; i < 20; ++i) navigationWidget(menu, root).onUnload();
    assert.equal(panel.buttons.length, 2);
    const next = navigationWidget(menu, root);
    const wrapped = panel.doUpdateButtons;
    panel.doUpdateButtons = function () { return wrapped.apply(this, arguments); };
    const later = panel.doUpdateButtons;
    next.onUnload();
    assert.equal(panel.doUpdateButtons, later, 'preserve later mod wrapper');
    panel.doUpdateButtons();
    assert.deepEqual(panel.visibleLabels, [label], 'inactive chained wrapper cannot remove again');
    const guarded = navigationWidget(menu, { Menu_mc: { navPanel: makePanel() } });
    assert.equal(guarded.hintPanel, undefined);
}
// Old JSON cannot resurrect custom buttons, even with a matching legacy entry.
for (const menu of ['MapMenu', 'StatsMenu', 'Sleep/Wait Menu', 'CharacterSheet', 'BestiaryMenu', 'AchievementMenu', 'MetaSkillsMenu']) {
    const host = makePanel();
    host.addButton({ text: '$Keep', controls: { keyCode: 56 } });
    const originalLayout = host.doUpdateButtons;
    for (const kind of ['anchor', 'panel']) {
        const config = { ...uiConfig, label: 'Next Menu', adapters: { [menu]: {
            kind, root: '', panel: 'panel', anchor: 'panel', legacy: { label: '$Keep', controls: 'controls' },
            align: 'left', placement: 'below', offsetX: 0, offsetY: 0, fontSize: 18
        } } };
        const widget = navigationWidget(menu, { panel: host, controls: { keyCode: 56 } }, config);
        widget.updateNavigation(...remembered, true);
        widget.onEnterFrame();
        assert.equal(widget.hintButton, undefined, menu);
        assert.equal(host.doUpdateButtons, originalLayout, menu);
        assert.equal(host.buttons[0].label, '$Keep');
        widget.onUnload();
    }
}
const unknown = navigationWidget('UnknownMenu', {});
priority = 0;
unknown.readBindingFile('[Other]\ncontrols.pc.switchTab=3\n[Input]\n controls.pc.switchTab = 42 ; comment\r\ncontrols.gamepad.switchTab = 272\n');
assert.deepEqual(remembered, [42, 272]);
flashGlobals.skyui = { util: { ConfigManager: { _config: { Input: { controls: { pc: { switchTab: 57 }, gamepad: { switchTab: 270 } } } } } } };
unknown.updateNavigation(42, 272, true);
assert.deepEqual(remembered, [57, 270]);
unknown.readBindingFile('[Input]\ncontrols.pc.switchTab=42\ncontrols.gamepad.switchTab=272');
assert.deepEqual(remembered, [57, 270], 'late file response cannot overwrite live binding');
assert.equal(unknown.navigationKey, 270);
assert.equal(unknown.resolvePath({}, '__proto__.constructor'), undefined);
assert.deepEqual(Object.keys(uiConfig.adapters).sort(), ['InventoryMenu', 'MagicMenu']);
console.log('Removal-only adapters: exact matches, rebuilds, cleanup, legacy configs, unchanged buttons and bindings.');

// CSF renders the selected skill tree in StatsMenu; its selector is CustomMenu
// under the logical MetaSkillsMenu key. Never make CSM a direct tree shortcut.
const csmEntries = [
    { menu: 'StatsMenu', label: 'Skills', available: true },
    { menu: 'MetaSkillsMenu', label: 'Custom Skills', available: true }
];
const csmTree = new Widget();
csmTree._parent = new MovieClip();
const beforeCsmSwitches = switches.length;
csmTree.initialize(api, 'StatsMenu', csmEntries, bounds);
assert.equal(csmTree._visible, true, 'navbar is present in the CSF StatsMenu host');
assert.equal(csmTree.tabs[1].isActive, false, 'CSM selector tab remains clickable inside the skill tree');
csmTree.tabs[1].onRelease();
assert.equal(switches.length, beforeCsmSwitches + 1);
assert.equal(switches[switches.length - 1], 'MetaSkillsMenu', 'click from a tree always targets the preselection');
const csmSelector = new Widget();
csmSelector._parent = new MovieClip();
csmSelector.initialize(api, 'MetaSkillsMenu', csmEntries, bounds);
assert.equal(csmSelector._visible, true, 'selector also receives its own navbar');
const beforeSelectorClick = switches.length;
csmSelector.tabs[1].onRelease();
assert.equal(switches.length, beforeSelectorClick, 'clicking CSM while already in the selector never enters a skill tree');
console.log('Custom Skills two-level navigation tests passed.');

const picker = new Widget();
picker._parent = new MovieClip();
picker.initialize(api, 'InventoryMenu', entries, bounds);
const beforePickerSwitches = switches.length;
assert.equal(picker.setPicker(true, 'InventoryMenu'), true);
assert.equal(picker.pickerActive, true);
assert.equal(picker.tabs[0].hoverTarget, 22, 'active menu starts expanded');
assert.ok(picker.tabs.slice(1).every(tab => tab.hoverTarget === 0), 'other labels start collapsed');
assert.equal(picker.tabs[0].visual.pickerMarker._visible, true);
picker.tabs[1].onRollOver();
picker.tabs[1].onRollOut();
picker.tabs[1].onRelease();
assert.equal(picker.tabs[1].hoverTarget, 0, 'mouse cannot change the picker selection');
assert.equal(switches.length, beforePickerSwitches, 'mouse cannot commit behind native hold selection');
assert.equal(picker.setPicker(true, 'ModMenu'), true);
assert.equal(picker.tabs[0].visual.pickerMarker._visible, false);
assert.equal(picker.tabs[1].visual.pickerMarker._visible, true, 'selected destination has its own marker');
assert.equal(picker.tabs[1].visual.icon.tint, 0xEEEEEE, 'navigation selection uses hover color');
assert.equal(picker.tabs[0].visual.icon.tint, 0xFFFFFF, 'active menu remains white');
assert.equal(picker.tabs[0].hoverTarget, 0, 'active menu collapses when it is not selected');
assert.equal(picker.tabs[1].hoverTarget, 22, 'new selection expands');
assert.equal(picker.tabs[2].hoverTarget, 0, 'unselected unavailable tab remains collapsed');
assert.equal(picker.tabs.filter(tab => tab.hoverTarget === 22).length, 1, 'only the selection is expanded');
picker.setPicker(true, 'InventoryMenu');
assert.equal(picker.tabs[1].hoverTarget, 0, 'previous selection collapses when moving away');
assert.equal(picker.tabs[1].visual.icon.tint, 0xAAAAAA, 'previous selection returns to normal gray');
assert.equal(picker.tabs[0].hoverTarget, 22);
picker.setPicker(true, 'MissingMenu');
assert.ok(picker.tabs.every(tab => !tab.visual.pickerMarker._visible), 'unavailable entries are never marked');
assert.ok(picker.tabs.every(tab => tab.hoverTarget === 0), 'no active-menu exception when selection is unavailable');
picker.setPicker(true, 'ModMenu');
picker.tabs.forEach(tab => { tab.progress = 17; });
picker.setPicker(false, '');
assert.equal(picker.pickerActive, false);
assert.ok(picker.tabs.every(tab => tab.hoverTarget === 0 && tab.progress === 17 && !tab.pickerSelected),
    'closing preserves positions for animated collapse/handoff instead of snapping');
assert.equal(switches.length, beforePickerSwitches, 'closing picker does not itself request navigation');
picker.beginNavigation();
assert.equal(picker.setPicker(true, 'ModMenu'), false, 'no picker while switching');
const uninitializedPicker = new Widget();
assert.equal(uninitializedPicker.setPicker(true, 'InventoryMenu'), false);
console.log('Hold picker presentation, mouse exclusion, unavailable targets and animation continuity passed.');
for (const [menu, linkage] of [['MagicMenu', 'NavbarMagicIcon'], ['BestiaryMenu', 'NavbarBestiaryIcon'],
    ['AchievementMenu', 'NavbarAchievementsIcon'], ['Sleep/Wait Menu', 'NavbarWaitIcon'],
    ['MetaSkillsMenu', 'NavbarCustomSkillsIcon'], ['CharacterSheet', 'NavbarCharacterIcon']]) {
    const iconWidget = new Widget();
    iconWidget._parent = new MovieClip();
    iconWidget.initialize(api, menu, [{ menu, label: menu, available: true }], bounds);
    const icon = iconWidget.tabs[0].visual.icon;
    assert.equal(icon.symbol, linkage, 'requested menu gets its own vector symbol');
    const expectedSize = menu === 'BestiaryMenu' ? 21 * 1.05 : 21;
    assert.ok(Math.abs(Math.max(icon._width, icon._height) - expectedSize) < 1e-6,
        'only the Bestiary book is 5% larger');
    assert.ok(Math.abs(icon._x + icon._width / 2 - 24) < 1e-6, 'icon stays centered horizontally');
    assert.ok(Math.abs(icon._y + icon._height / 2 - 22) < 1e-6, 'icon stays centered vertically');
    assert.equal(icon.tint, 0xFFFFFF, 'active icons retain the white palette');
}
for (const factor of [0.25, 0.8, 1, 1.25, 3]) {
    const scaled = new Widget();
    scaled._parent = new MovieClip();
    scaled.initialize(api, 'InventoryMenu', entries, { ...bounds, scale: factor });
    const actualScale = Math.min(factor * 0.8, bounds.bottom / (scaled.totalHeight + 112));
    assert.equal(scaled._xscale, actualScale * 100);
    assert.equal(scaled._yscale, actualScale * 100, 'scale the entire sidebar uniformly, including hint padding');
    assert.equal(scaled._x, bounds.right);
    assert.equal(scaled._y, (bounds.bottom - scaled.totalHeight * actualScale) / 2);
    scaled.tabs[1].onRollOver();
    time += 100;
    scaled.onEnterFrame();
    scaled.layout(0, 0, 1280, 720);
    assert.equal(scaled._xscale, actualScale * 100, 'periodic layout and hover retain configured size');
    assert.ok(Math.abs(scaled.tabs[1].visual.icon._height - 21) < 1e-6, 'child icon is not double-scaled');
    scaled.layout(0, 0, 1280, 20);
    assert.ok(scaled.totalHeight * scaled._yscale / 100 <= 20 + 1e-6, 'large rail still fits short viewport');
    scaled.layout(0, 0, 10, 720);
    assert.ok(scaled.totalWidth * scaled._xscale / 100 <= 10 + 1e-6, 'expanded rail stays onscreen');
}
for (const scale of [undefined, 0, -1, NaN, Infinity, '1.25', 0.24, 3.01]) {
    const fallback = new Widget();
    fallback._parent = new MovieClip();
    fallback.initialize(api, 'InventoryMenu', entries, { ...bounds, scale });
    assert.equal(fallback._xscale, 80, 'invalid or missing Flash setting uses the new SWF baseline');
}
console.log('Configurable whole-sidebar scaling, persistent layout, viewport caps and darker idle palette passed.');

delete flashGlobals.skyui;
const keyWidget = navigationWidget('StatsMenu', {});
assert.equal(keyWidget.keyHints.holdLabel.text, '(hold)');
assert.equal(keyWidget.keyHints.holdLabel.autoSize, 'center', 'translated hold labels grow around the same center');
const germanHint = new Widget();
germanHint._parent = new MovieClip();
germanHint.initialize({ ...api, Translate: key => key === '$SkyUINavbar_Hold' ? '(halten)' : key }, 'InventoryMenu', entries, bounds);
assert.equal(germanHint.keyHints.holdLabel.text, '(halten)', 'SWF key resolves through generic translation API');
assert.ok(germanHint.keyHints.holdLabel._x + germanHint.keyHints.holdLabel._width <= 0,
    'longer translated hint stays inside the right screen edge');
const longHint = new Widget();
longHint._parent = new MovieClip();
longHint.initialize({ ...api, Translate: key => key === '$SkyUINavbar_Hold' ? 'A much longer translated hold instruction' : key }, 'InventoryMenu', entries, bounds);
longHint.layout(0, 0, 100, 720);
assert.ok(longHint.keyHints.holdLabel._width * longHint._xscale / 100 <= 100 + 1e-6,
    'viewport fitting includes long translated hints');
const missingHint = new Widget();
missingHint._parent = new MovieClip();
missingHint.initialize({ ...api, Translate: undefined }, 'InventoryMenu', entries, bounds);
assert.equal(missingHint.keyHints.holdLabel.text, '$SkyUINavbar_Hold', 'fallback uses a standard translation key, not hardcoded English');
assert.equal(keyWidget.keyHints.holdLabel._y, -52, 'hold label moved down 4px');
assert.equal(keyWidget.keyHints.holdLabel._height, 24, '16px Skyrim font has enough vertical room');
assert.ok(keyWidget.keyHints.holdLabel._y + 16 + 4 <= -30, 'font height plus padding stays above upper glyph');
assert.equal(keyWidget.keyHints.holdLabel.selectable, false);
assert.equal(keyWidget.keyHints.holdLabel.filters.length, 1, 'hold hint gets one shadow');
assert.deepEqual(keyWidget.keyHints.holdLabel.filters[0].args,
    [2, 45, 0x000000, 1, 2, 2, 1, 1, false, false, false], 'matches SkyUI ContainerMenu text shadow');
assert.equal(keyWidget.keyHints.upper.filters, undefined, 'button art is unchanged');
assert.equal(keyWidget.tabs[0].visual.label.filters, undefined, 'shadow applies only to hold hint');
assert.equal(keyWidget.keyHints.lower.onRelease, undefined, 'hints are decorative');
for (const [pc, pad, gamepad, expected] of [[56, 271, false, 56], [56, 271, true, 271],
    [42, 274, false, 42], [42, 274, true, 274], [-1, 271, false, 282]]) {
    keyWidget.updateNavigation(pc, pad, gamepad);
    assert.equal(keyWidget.keyHints._visible, true, 'hints work without any host button panel');
    for (const [name, centerY] of [['upper', -18], ['lower', keyWidget.totalHeight + 18]]) {
        const art = keyWidget.keyHints[name];
        assert.equal(art.frame, expected, 'same direct SkyUI key-code frame in both hints');
        assert.equal(art.symbol, 'ButtonArt');
        assert.equal(art._x + 14 * art._xscale / 100, -24, 'center on collapsed rail');
        assert.equal(art._y + 12 * art._yscale / 100, centerY);
    }
}
keyWidget.layout(0, 0, 1280, 100);
const hintScale = keyWidget._yscale / 100;
assert.ok(keyWidget._y - 56 * hintScale >= -1e-6, 'hold text fits short viewport');
assert.ok(keyWidget._y + (keyWidget.totalHeight + 56) * hintScale <= 100 + 1e-6,
    'lower hint fits short viewport');
assert.equal(keyWidget.containsPoint(1, 2), false, 'hints never capture host clicks');
const offsetArt = new MovieClip();
offsetArt.getBounds = () => ({ xMin: -10, xMax: 50, yMin: -5, yMax: 15 });
assert.equal(keyWidget.positionKeyArt(offsetArt, 56, -18), true);
assert.ok(Math.abs(offsetArt._x + 20 * offsetArt._xscale / 100 + 24) < 1e-6,
    'nonzero-origin replacement glyphs stay centered');
assert.equal(offsetArt._xscale, offsetArt._yscale, 'wide keys retain aspect ratio');
console.log('Navbar ButtonArt hints: keyboard/controller rebinding, placement, viewport safety and no host buttons passed.');

flashGlobals.skyui = { util: { ConfigManager: { _config: { Input: { controls: { pc: { switchTab: 57 }, gamepad: { switchTab: 270 } } } } } } };
const builtInKeys = navigationWidget('StatsMenu', {}, undefined);
builtInKeys.updateNavigation(42, 274, false);
assert.equal(builtInKeys.navigationKey, 57, 'SkyUI bindings remain authoritative without a separate UI config');
const noHints = new Widget();
noHints._parent = new MovieClip();
noHints.initialize(navigationAPI, 'StatsMenu', entries, { ...bounds, showButtonHints: false });
noHints.configureNavigation(undefined, {});
noHints.updateNavigation(56, 271, false);
assert.equal(noHints.keyHints, undefined, 'disabled hints create neither glyphs nor hold text');
assert.equal(noHints.navigationKey, 57, 'hiding hints does not disable keyboard input');
noHints.updateNavigation(56, 271, true);
assert.equal(noHints.navigationKey, 270, 'hiding hints does not disable controller input');
assert.equal(noHints.keyHints, undefined, 'periodic binding updates cannot recreate hidden hints');
noHints.layout(0, 0, 1280, noHints.totalHeight);
assert.equal(noHints._yscale, 80, 'new SWF baseline without reserved hint space when hidden');
delete flashGlobals.skyui;

// Menu naming is a SWF concern, never supplied by JSON or the loader.
const names = {
    InventoryMenu: 'Inventory', MagicMenu: 'Magic', MapMenu: 'Map',
    BestiaryMenu: 'Bestiary', StatsMenu: 'Skills', MetaSkillsMenu: 'CustomSkills',
    'Sleep/Wait Menu': 'Wait', CharacterSheet: 'Character', AchievementMenu: 'Achievements'
};
const translatedWidget = new Widget();
translatedWidget._parent = new MovieClip();
translatedWidget.initialize({ ...api, Translate: key => 'localized:' + key }, 'InventoryMenu',
    Object.keys(names).map(menu => ({ menu, available: true, label: 'ignored stale label' })), bounds);
for (const tab of translatedWidget.tabs) {
    assert.equal(tab.visual.label.text, 'localized:$SkyUINavbar_' + names[tab.menuName]);
    assert.equal(tab.visual.label._y, 9, 'tab text moved down 2 SWF pixels');
    assert.equal(tab.visual.label.noTranslate, true, 'already localized text must not be translated again');
    assert.equal(tab.slideWidth, tab.visual.label._width + 18, 'measure translated label');
}
assert.equal(translatedWidget.labelForMenu('ExtraMenu'), 'localized:$SkyUINavbar_ExtraMenu');
translatedWidget.api = { ...api, Translate: key => key };
assert.equal(translatedWidget.labelForMenu('ExtraMenu'), 'ExtraMenu', 'untranslated extra menu keeps readable ID');
assert.equal(translatedWidget.labelForMenu('InventoryMenu'), '$SkyUINavbar_Inventory', 'missing built-in translation remains diagnosable');
automaticTranslations = { Inventory: 'INVENTORY', Magic: 'MAGIC', '(hold)': '(HOLD)' };
const caseWidget = new Widget();
caseWidget._parent = new MovieClip();
caseWidget.initialize({ ...api, Translate: key => ({
    '$SkyUINavbar_Inventory': 'Inventory', '$SkyUINavbar_Magic': 'Magic', '$SkyUINavbar_Hold': '(hold)'
}[key] || key) }, 'InventoryMenu', [{ menu: 'InventoryMenu', available: true }, { menu: 'MagicMenu', available: true }], bounds);
assert.equal(caseWidget.tabs[0].visual.label.text, 'Inventory', 'vanilla uppercase translation cannot replace our label');
assert.equal(caseWidget.tabs[1].visual.label.text, 'Magic');
assert.equal(caseWidget.keyHints.holdLabel.text, '(hold)');
automaticTranslations = {};
