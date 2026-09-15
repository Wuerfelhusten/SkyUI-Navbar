/* FFDec AS2 requires explicit object/static member access.
 * Background #78 is imported from navbar-background.svg; sprite #79 uses nine-slice sizing.
 */
class NavbarWidget extends MovieClip
{
    var api;
    var tabs;
    var initialized;
    var currentMenu;
    var transitioning;
    var requestTime;
    var totalWidth;
    var lastFrameTime;
    var pressedTab;
    var transitionCover;
    var totalHeight;
    var navigation;
    var hostRoot;
    var hintAdapter;
    var hintRoot;
    var hintPanel;
    var hintHooks;
    var removedHints;
    var navigationKey;
    var navigationActive;
    var navigationGamepad;
    var pickerActive;
    var navbarScale;
    var keyHints;
    var keyHintFrame;
    var showButtonHints;
    var holdText;

    function NavbarWidget()
    {
        super();
        this.initialized = false;
    }

    function setTransitionCover(alpha, left, top, right, bottom)
    {
        if (this.transitionCover == undefined) {
            this.transitionCover = this.createEmptyMovieClip("transitionCover", 20000);
        }
        var cover = this.transitionCover;
        cover.clear();
        cover.beginFill(0x000000, 100);
        cover.moveTo(left - 1, top - 1);
        cover.lineTo(right + 1, top - 1);
        cover.lineTo(right + 1, bottom + 1);
        cover.lineTo(left - 1, bottom + 1);
        cover.lineTo(left - 1, top - 1);
        cover.endFill();
        cover._alpha = Math.max(0, Math.min(100, alpha));
    }

    function initialize(a_api, a_menu, a_entries, a_bounds)
    {
        this.api = a_api;
        this.currentMenu = a_menu;
        this.transitioning = false;
        this.pressedTab = null;
        this.pickerActive = false;
        this.showButtonHints = a_bounds.showButtonHints !== false;
        this.holdText = this.translate("$SkyUINavbar_Hold");
        this.navbarScale = typeof a_bounds.scale == "number" && isFinite(a_bounds.scale) &&
            a_bounds.scale >= 0.25 && a_bounds.scale <= 3 ? a_bounds.scale : 1;
        this._visible = true;
        this._alpha = 100;
        if (!this.initialized) {
            this.tabs = [];
            this.buildTabs(a_entries);
            this.buildKeyHints();
            this.initialized = true;
        }
        this.lastFrameTime = getTimer();
        this.layout(a_bounds.left, a_bounds.top, a_bounds.right, a_bounds.bottom);
        this.api.Ready("initialized:" + this.tabs.length);
        return this.tabs.length;
    }

    function translate(key)
    {
        if (typeof this.api.Translate == "function") {
            var text = this.api.Translate(key);
            if (typeof text == "string" && text.length > 0) { return text; }
        }
        return key;
    }

    function labelForMenu(menu)
    {
        var keys = {
            InventoryMenu: "$SkyUINavbar_Inventory",
            MagicMenu: "$SkyUINavbar_Magic",
            MapMenu: "$SkyUINavbar_Map",
            BestiaryMenu: "$SkyUINavbar_Bestiary",
            StatsMenu: "$SkyUINavbar_Skills",
            MetaSkillsMenu: "$SkyUINavbar_CustomSkills",
            CharacterSheet: "$SkyUINavbar_Character",
            AchievementMenu: "$SkyUINavbar_Achievements"
        };
        keys["Sleep/Wait Menu"] = "$SkyUINavbar_Wait";
        if (keys[menu] != undefined) { return this.translate(keys[menu]); }
        // Extra menus can supply a translation without adding config fields.
        var key = "$SkyUINavbar_" + menu;
        var text = this.translate(key);
        return text == key ? menu : text;
    }

    function iconForMenu(menu)
    {
        if (menu == "InventoryMenu") { return "NavbarInventoryIcon"; }
        if (menu == "MapMenu") { return "NavbarMapIcon"; }
        if (menu == "StatsMenu") { return "NavbarSkillsIcon"; }
        if (menu == "MagicMenu") { return "NavbarMagicIcon"; }
        if (menu == "BestiaryMenu") { return "NavbarBestiaryIcon"; }
        if (menu == "AchievementMenu") { return "NavbarAchievementsIcon"; }
        if (menu == "Sleep/Wait Menu") { return "NavbarWaitIcon"; }
        if (menu == "MetaSkillsMenu") { return "NavbarCustomSkillsIcon"; }
        if (menu == "CharacterSheet") { return "NavbarCharacterIcon"; }
        // Unknown menus reuse the map artwork instead of embedding a duplicate.
        return "NavbarMapIcon";
    }

    function buildTabs(entries)
    {
        var position = 0;
        this.totalWidth = 48;
        for (var i = 0; i < entries.length; i++) {
            var tab = this.createEmptyMovieClip("tab" + i, 100 + i);
            tab.menuName = entries[i].menu;
            tab.isActive = tab.menuName == this.currentMenu;
            tab.available = entries[i].available;
            tab.isHover = false;
            tab.owner = this;
            tab.hoverTarget = 0;
            tab.restoredHover = false;
            tab.useHandCursor = tab.available && !tab.isActive;
            tab.createEmptyMovieClip("visual", 1);
            tab.visual.attachMovie("NavbarTabBackground", "background", 1);
            tab.visual.background._width = 55;
            tab.visual.background._height = 44;
            tab.visual.attachMovie(this.iconForMenu(tab.menuName), "icon", 2);
            var iconBounds = tab.visual.icon.getBounds(tab.visual.icon);
            var iconWidth = iconBounds.xMax - iconBounds.xMin;
            var iconHeight = iconBounds.yMax - iconBounds.yMin;
            var iconSize = tab.menuName == "BestiaryMenu" ? 21 * 1.05 : 21;
            var iconScale = iconSize / Math.max(iconWidth, iconHeight);
            if (iconWidth > 0 && iconHeight > 0) {
                tab.visual.icon._width = iconWidth * iconScale;
                tab.visual.icon._height = iconHeight * iconScale;
                tab.visual.icon._x = 24 - (iconBounds.xMin + iconBounds.xMax) * iconScale / 2;
                tab.visual.icon._y = 22 - (iconBounds.yMin + iconBounds.yMax) * iconScale / 2;
            }
            tab.visual.createTextField("label", 3, 48, 9, 80, 30);
            tab.visual.label.selectable = false;
            // Already localized by our bridge; avoid a second vanilla translation.
            tab.visual.label.noTranslate = true;
            tab.visual.label.embedFonts = false;
            tab.visual.label.autoSize = "left";
            tab.visual.label.text = this.labelForMenu(tab.menuName);
            tab.visual.label.setTextFormat(new TextFormat(
                "$EverywhereMediumFont", 20, 0xAAAAAA, false, false, false,
                null, null, "left"));
            var marker = tab.visual.createEmptyMovieClip("pickerMarker", 4);
            marker.beginFill(0xFFFFFF, 100);
            marker.moveTo(5, 11);
            marker.lineTo(7, 11);
            marker.lineTo(7, 33);
            marker.lineTo(5, 33);
            marker.lineTo(5, 11);
            marker.endFill();
            marker._visible = false;
            tab.slideWidth = tab.visual.label._width + 18;
            tab.tabWidth = 48 + tab.slideWidth;
            tab.visual._y = 0;
            tab.progress = 0;
            tab._x = 0;
            tab._y = position;
            position += 50;
            this.totalWidth = Math.max(this.totalWidth, tab.tabWidth);
            tab.redraw = this.redrawTab;
            tab.onRollOver = this.onTabRollOver;
            tab.onRollOut = this.onTabRollOut;
            tab.onReleaseOutside = this.onTabRollOut;
            tab.onRelease = this.onTabRelease;
            if (typeof entries[i].animationY == "number" && !isNaN(entries[i].animationY)) {
                tab.progress = Math.max(0, Math.min(22, entries[i].animationY));
                tab.hoverTarget = typeof entries[i].hoverTarget == "number" && !isNaN(entries[i].hoverTarget) ?
                    Math.max(0, Math.min(22, entries[i].hoverTarget)) : 0;
                tab.isHover = entries[i].isHover == true;
                tab.restoredHover = tab.isHover;
            }
            tab.redraw();
            this.tabs.push(tab);
        }
        this.totalHeight = entries.length > 0 ? position - 6 : 0;
        this._visible = entries.length > 0;
    }

    function layout(left, top, right, bottom)
    {
        if (!this.initialized || this.totalHeight <= 0 || right <= left || bottom <= top) {
            return;
        }
        var origin = { x: left, y: top };
        var corner = { x: right, y: bottom };
        this._parent.globalToLocal(origin);
        this._parent.globalToLocal(corner);
        var availableWidth = corner.x - origin.x;
        var availableHeight = corner.y - origin.y;
        if (!isFinite(availableWidth) || !isFinite(availableHeight) || availableWidth <= 0 || availableHeight <= 0) {
            return;
        }
        // Reserve room for both hints while keeping the menu rows centered.
        var hintMargin = this.showButtonHints ? 112 : 0;
        // SWF-native baseline: config 100% matches the former 80% sidebar.
        var scale = Math.min(this.navbarScale * 0.8, availableWidth / this.totalWidth, availableHeight / (this.totalHeight + hintMargin));
        this._xscale = scale * 100;
        this._yscale = scale * 100;
        this._x = corner.x;
        this._y = (origin.y + corner.y - this.totalHeight * scale) / 2;
    }

    function captureAnimationState()
    {
        var state = [];
        if (!this.initialized) { return state; }
        for (var i = 0; i < this.tabs.length; i++) {
            var tab = this.tabs[i];
            state.push({ menu: tab.menuName, position: tab.progress,
                target: tab.hoverTarget, hover: tab.isHover });
        }
        return state;
    }

    function buildKeyHints()
    {
        if (!this.showButtonHints) { return; }
        this.keyHints = this.createEmptyMovieClip("keyHints", 90);
        this.keyHints.attachMovie("ButtonArt", "upper", 1);
        this.keyHints.attachMovie("ButtonArt", "lower", 2);
        this.keyHints.createTextField("holdLabel", 3, -48, -52, 48, 24);
        var label = this.keyHints.holdLabel;
        label.selectable = false;
        label.noTranslate = true;
        label.autoSize = "center";
        label.text = this.holdText;
        var format = new TextFormat();
        format.font = "$EverywhereMediumFont";
        format.size = 16;
        format.align = "center";
        format.color = 0xEEEEEE;
        label.setTextFormat(format);
        // SkyUI ContainerMenu text shadow: black, 2px at 45 degrees, 2px blur.
        label.filters = [new flash.filters.DropShadowFilter(2, 45, 0x000000, 1, 2, 2, 1, 1, false, false, false)];
        // Long translations may grow left, but never beyond the screen's right edge.
        label._x = Math.min(-24 - label._width / 2, -label._width);
        this.totalWidth = Math.max(this.totalWidth, label._width);
        this.keyHints._visible = false;
    }

    function positionKeyArt(art, frame, centerY)
    {
        if (art == undefined) { return false; }
        // SkyUI MappedButton uses the SKSE key code directly as the frame.
        art.gotoAndStop(frame);
        var bounds = art.getBounds(art);
        var width = bounds.xMax - bounds.xMin;
        var height = bounds.yMax - bounds.yMin;
        if (!(width > 0) || !(height > 0)) { return false; }
        var scale = Math.min(32 / width, 24 / height);
        art._xscale = scale * 100;
        art._yscale = scale * 100;
        art._x = -24 - (bounds.xMin + width / 2) * scale;
        art._y = centerY - (bounds.yMin + height / 2) * scale;
        return true;
    }

    function updateKeyHints(key)
    {
        if (this.keyHints == undefined || this.keyHintFrame === key) { return; }
        var frame = typeof key == "number" && isFinite(key) && key > 0 && key <= 281 &&
            Math.floor(key) == key ? key : 282;
        var upper = this.positionKeyArt(this.keyHints.upper, frame, -18);
        var lower = this.positionKeyArt(this.keyHints.lower, frame, this.totalHeight + 18);
        this.keyHints._visible = upper && lower && this.tabs.length > 0;
        if (upper && lower) { this.keyHintFrame = key; }
    }

    function reconcileRestoredHover(x, y)
    {
        if (!this.initialized || this.transitioning || this.pickerActive) { return; }
        for (var i = 0; i < this.tabs.length; i++) {
            var tab = this.tabs[i];
            if (tab.restoredHover && !tab.hitTest(x, y, true)) {
                tab.onRollOut();
            }
        }
    }

    function containsPoint(x, y)
    {
        if (!this.initialized || !this._visible) { return false; }
        for (var i = 0; i < this.tabs.length; i++) {
            if (this.tabs[i].hitTest(x, y, true)) { return true; }
        }
        return false;
    }

    function nativeButton(button, down, x, y)
    {
        if (button != 0) { return; }
        var pressed = this.pressedTab;
        this.pressedTab = null;
        if (!this.initialized || !this._visible || this.transitioning || this.pickerActive) { return; }
        for (var i = 0; i < this.tabs.length; i++) {
            var tab = this.tabs[i];
            if (!tab.hitTest(x, y, true)) { continue; }
            if (down) {
                this.pressedTab = tab;
                tab.onRollOver();
            } else if (pressed == tab) {
                tab.onRelease();
            }
            return;
        }
    }

    function redrawTab()
    {
        var color = this.isActive ? 0xFFFFFF : (this.isHover || this.pickerSelected ? 0xEEEEEE : 0xAAAAAA);
        this.visual.pickerMarker._visible = this.pickerSelected == true;
        this.visual.label.textColor = color;
        var iconColor = new Color(this.visual.icon);
        iconColor.setRGB(color);
        var visibleWidth = 48 + this.slideWidth * this.progress / 22;
        this.visual._x = -visibleWidth;
        // Nine-slice keeps the 5px right inset fixed; 7px overdraw hides the open ends beyond the viewport.
        this.visual.background._width = visibleWidth + 7;
        this.visual.label._alpha = this.progress * 100 / 22;
        this.visual.label._visible = this.progress > 0;
    }

    function onTabRollOver()
    {
        if (this.owner.transitioning || this.owner.pickerActive) { return; }
        this.isHover = true;
        this.restoredHover = false;
        this.hoverTarget = 22;
        this.redraw();
    }

    function onTabRollOut()
    {
        if (this.owner.transitioning || this.owner.pickerActive) { return; }
        this.isHover = false;
        this.restoredHover = false;
        this.hoverTarget = 0;
        this.redraw();
    }

    function onTabRelease()
    {
        if (this.owner.transitioning || this.owner.pickerActive || this.isActive || !this.available) { return; }
        this.owner.transitioning = this.owner.api.SwitchTo(this.menuName) == true;
        this.owner.requestTime = getTimer();
    }

    function setPicker(active, selected)
    {
        if (!this.initialized || (active && (this.transitioning || !this._visible))) { return false; }
        if (!active && !this.pickerActive) { return true; }
        this.pickerActive = active == true;
        this.pressedTab = null;
        for (var i = 0; i < this.tabs.length; i++) {
            var tab = this.tabs[i];
            tab.pickerSelected = this.pickerActive && tab.available && tab.menuName == selected;
            tab.restoredHover = false;
            tab.isHover = false;
            tab.hoverTarget = tab.pickerSelected ? 22 : 0;
            tab.redraw();
        }
        return true;
    }

    function onEnterFrame()
    {
        if (!this.initialized) { return; }
        var now = getTimer();
        if (this.transitioning && now - this.requestTime > 6000) {
            this.transitioning = false;
        }
        var step = Math.min(100, now - this.lastFrameTime) * 22 / 200;
        this.lastFrameTime = now;
        if (this.transitioning) { return; }
        for (var i = 0; i < this.tabs.length; i++) {
            var tab = this.tabs[i];
            if (tab.progress < tab.hoverTarget) {
                tab.progress = Math.min(tab.hoverTarget, tab.progress + step);
            } else if (tab.progress > tab.hoverTarget) {
                tab.progress = Math.max(tab.hoverTarget, tab.progress - step);
            }
            tab.redraw();
        }
    }

    // Like Show in UI: the loaded movie owns the host integration, not SKSE.
    // All host paths and legacy-entry identities are supplied by the UI JSON.
    function resolvePath(root, path)
    {
        if (path == "") { return root; }
        if (typeof path != "string") { return undefined; }
        var parts = path.split(".");
        for (var i = 0; i < parts.length; i++) {
            if (root == undefined || parts[i] == "__proto__" || parts[i] == "prototype" || parts[i] == "constructor") { return undefined; }
            root = root[parts[i]];
        }
        return root;
    }

    function configureNavigation(options, root)
    {
        if (options == undefined) { options = this.navigationDefaults(); }
        this.detachHints();
        this.navigation = options;
        this.hostRoot = root;
        this.navigationActive = true;
        this.hintAdapter = options.adapters == undefined ? undefined : options.adapters[this.currentMenu];
        var owner = this;
        if (typeof options.skyuiConfig == "string") {
            var loader = new LoadVars();
            loader.onData = function(data) {
                if (owner.navigationActive && typeof data == "string") { owner.readBindingFile(data); }
            };
            loader.load(options.skyuiConfig);
        }
    }

    // View-in-Menu style contracts stay with the UI, never in user settings.
    function navigationDefaults()
    {
        return {
            skyuiConfig: "skyui/config.txt",
            globalBindingRoot: "skyui.util.ConfigManager._config.Input.controls",
            adapters: {
                InventoryMenu: {
                    kind: "panel", root: "Menu_mc", panel: "navPanel",
                    required: ["inventoryLists", "itemCard", "openMagicMenu"],
                    bindingRoot: "_config.Input.controls",
                    legacy: {label: "$Magic", controls: "_switchControls"}
                },
                MagicMenu: {
                    kind: "panel", root: "Menu_mc", panel: "navPanel",
                    required: ["inventoryLists", "itemCard", "openInventoryMenu"],
                    bindingRoot: "_config.Input.controls",
                    legacy: {label: "$Inventory", controls: "_switchControls"}
                }
            }
        };
    }

    function readBindingFile(data)
    {
        var lines = data.split("\n");
        var section = "";
        var pc;
        var pad;
        for (var i = 0; i < lines.length; i++) {
            var line = lines[i].split(";")[0].split("\r").join("").split(" ").join("").split("\t").join("");
            if (line.charAt(0) == "[") { section = line; }
            if (section != "[Input]") { continue; }
            var pair = line.split("=");
            if (pair[0] == "controls.pc.switchTab") { pc = Number(pair[1]); }
            if (pair[0] == "controls.gamepad.switchTab") { pad = Number(pair[1]); }
        }
        this.api.SetCycleKeys(pc, pad, true);
    }

    function updateNavigation(pc, pad, gamepad)
    {
        if (!this.navigationActive) { return; }
        var adapter = this.hintAdapter;
        var root = adapter == undefined ? undefined : this.resolvePath(this.hostRoot, adapter.root);
        if (this.hintRoot != undefined && root != this.hintRoot) {
            this.detachHints();
            this.navigationActive = true;
        }
        var controls = adapter == undefined ? undefined : this.resolvePath(root, adapter.bindingRoot);
        if (controls == undefined) { controls = this.resolvePath(_global, this.navigation.globalBindingRoot); }
        // The native bridge remembers the last binding across movie instances.
        if (controls != undefined && this.api.SetCycleKeys(controls.pc.switchTab, controls.gamepad.switchTab)) {
            pc = controls.pc.switchTab;
            pad = controls.gamepad.switchTab;
        }
        this.navigationKey = gamepad ? pad : pc;
        this.navigationGamepad = gamepad;
        this.updateKeyHints(this.navigationKey);
        if (adapter == undefined || root == undefined || adapter.legacy == undefined ||
            (this.currentMenu != "InventoryMenu" && this.currentMenu != "MagicMenu")) { return; }
        for (var i = 0; adapter.required != undefined && i < adapter.required.length; i++) {
            if (this.resolvePath(root, adapter.required[i]) == undefined) { return; }
        }
        this.hintRoot = root;
        if (adapter.kind == "panel") {
            var panel = this.resolvePath(root, adapter.panel);
            if (panel != this.hintPanel) {
                this.detachHints();
                this.navigationActive = true;
                this.hintRoot = root;
                this.installPanel(panel);
            }
            if (this.hintPanel != undefined) { this.hintPanel.doUpdateButtons(); }
        }
    }

    function beginNavigation()
    {
        this.setPicker(false, "");
        this.transitioning = true;
        this.requestTime = getTimer();
    }

    function restoreLegacyHints()
    {
        for (var i = 0; this.removedHints != undefined && i < this.removedHints.length; i++) {
            var item = this.removedHints[i];
            if (item.button.label == "" && item.button._keyCodes.length == 1 && item.button._keyCodes[0] == item.key) {
                item.button.label = item.label;
                item.button._visible = item.visible;
            }
        }
        this.removedHints = [];
    }

    function installPanel(panel)
    {
        if (panel == undefined || !(panel.buttons instanceof Array) || typeof panel.addButton != "function" ||
            typeof panel.clearButtons != "function" || typeof panel.doUpdateButtons != "function" ||
            typeof panel.buttonRenderer != "string" || typeof panel._buttonCount != "number") { return; }
        this.hintPanel = panel;
        this.removedHints = [];
        var owner = this;
        var hooks = { active: true, clear: panel.clearButtons, layout: panel.doUpdateButtons };
        hooks.clearWrapper = function() {
            if (hooks.active) { owner.restoreLegacyHints(); }
            return hooks.clear.apply(this, arguments);
        };
        hooks.layoutWrapper = function() {
            if (hooks.active) { owner.removeLegacyHints(); }
            return hooks.layout.apply(this, arguments);
        };
        this.hintHooks = hooks;
        panel.clearButtons = hooks.clearWrapper;
        panel.doUpdateButtons = hooks.layoutWrapper;
        if (typeof panel.hideButtons == "function" && typeof panel.showButtons == "function") {
            hooks.hide = panel.hideButtons;
            hooks.show = panel.showButtons;
            hooks.hideWrapper = function() {
                if (hooks.active) { hooks.hidden = true; }
                return hooks.hide.apply(this, arguments);
            };
            hooks.showWrapper = function() {
                if (hooks.active) { hooks.hidden = false; }
                var result = hooks.show.apply(this, arguments);
                if (hooks.active) { this.doUpdateButtons(); }
                return result;
            };
            panel.hideButtons = hooks.hideWrapper;
            panel.showButtons = hooks.showWrapper;
        }
    }

    function removeLegacyHints()
    {
        var panel = this.hintPanel;
        if (this.hintHooks.hidden) { return; }
        var legacy = this.hintAdapter.legacy;
        var controls = legacy == undefined ? undefined : this.resolvePath(this.hintRoot, legacy.controls);
        for (var i = 0; i < panel._buttonCount; i++) {
            var button = panel.buttons[i];
            // No index-based deletion, no translated-label guess, no unrelated shortcuts.
            if (legacy != undefined && controls != undefined && typeof controls.keyCode == "number" &&
                button.label === legacy.label && button._keyCodes.length == 1 && button._keyCodes[0] === controls.keyCode) {
                this.removedHints.push({ button: button, label: button.label, key: controls.keyCode, visible: button._visible });
                button._visible = false;
                button.label = "";
            }
        }
    }

    function detachHints()
    {
        this.navigationActive = false;
        var panel = this.hintPanel;
        var hooks = this.hintHooks;
        if (panel != undefined && hooks != undefined) {
            this.restoreLegacyHints();
            hooks.active = false;
            if (panel.clearButtons == hooks.clearWrapper) { panel.clearButtons = hooks.clear; }
            if (panel.doUpdateButtons == hooks.layoutWrapper) { panel.doUpdateButtons = hooks.layout; }
            if (hooks.hideWrapper != undefined && panel.hideButtons == hooks.hideWrapper) { panel.hideButtons = hooks.hide; }
            if (hooks.showWrapper != undefined && panel.showButtons == hooks.showWrapper) { panel.showButtons = hooks.show; }
            panel.doUpdateButtons();
        }
        this.hintRoot = undefined;
        this.hintPanel = undefined;
        this.hintHooks = undefined;
    }

    function onUnload()
    {
        this.detachHints();
    }
}
