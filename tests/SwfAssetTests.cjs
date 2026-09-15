const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const assert = require('node:assert/strict');
const [ffdec, base, final] = process.argv.slice(2);
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'navbar-swf-test-'));
try {
    const xml = [base, final].map((swf, i) => {
        const output = path.join(temporary, `${i}.xml`);
        execFileSync(ffdec, ['-swf2xml', swf, output], { windowsHide: true, timeout: 30000 });
        return fs.readFileSync(output, 'utf8');
    });
    function shape(source, id) {
        const match = source.match(new RegExp(`<item type="DefineShape4Tag"[^>]*shapeId="${id}"[^>]*>[\\s\\S]*?\n    </item>`));
        assert.ok(match, `shape ${id} is present`);
        return match[0];
    }
    const background = shape(xml[1], 78);
    assert.ok(xml[1].includes('url="skyui/buttonart.swf"'), 'own navbar imports the installed SkyUI button art');
    assert.ok(xml[1].includes('<item>ButtonArt</item>'), 'ButtonArt linkage is available without host-menu edits');
    assert.notEqual(shape(xml[0], 78), background, 'SVG replaces the empty build placeholder');
    const backgroundBounds = background.match(/<shapeBounds[^>]+>/)[0];
    for (const [key, value] of Object.entries({ Xmin: 0, Ymin: 0, Xmax: 960, Ymax: 880 })) {
        assert.ok(backgroundBounds.includes(`${key}="${value}"`), 'background has native 48x44 bounds');
    }
    assert.ok(background.includes('alpha="113"'), 'translucent fill retained');
    assert.ok(background.includes('width="40"'), '2px frame stroke retained');
    assert.ok(background.includes('red="156"'), 'gray frame retained');
    assert.ok(!background.includes('CurvedEdgeRecord'), 'background has square corners');
    const edges = (background.match(/<item type="StraightEdgeRecord"[^>]*\/>/g) || [])
        .filter(edge => !edge.includes('deltaX="0" deltaY="0"'));
    assert.equal(edges.length, 7,
        'four fill edges plus three frame edges: the frame must remain open');
    assert.ok(background.includes('noClose="true"'), 'renderer must not close the open stroke');
    const grid = xml[1].match(/<item type="DefineScalingGridTag"[^>]*characterId="79"[^>]*>[\s\S]*?<\/item>/);
    assert.ok(grid, 'nine-slice grid is embedded on the background sprite, not the icon/text');
    for (const [key, value] of Object.entries({ Xmin: 140, Xmax: 820, Ymin: 140, Ymax: 740 })) {
        assert.ok(grid[0].includes(`${key}="${value}"`), '7px fixed edges enclose the inner frame');
    }
    for (const source of xml) {
        assert.ok(!source.includes('shapeId="80"'), 'duplicate map/placeholder shape is absent');
        assert.ok(!source.includes('spriteId="81"'), 'duplicate map/placeholder sprite is absent');
        assert.ok(!source.includes('NavbarPlaceholderIcon'), 'obsolete placeholder linkage is absent');
        assert.equal((source.match(/shapeId="84"/g) || []).length, 1, 'map shape is defined exactly once');
    }
    for (const [name, id] of [['Inventory', 82], ['Map', 84], ['Skills', 86], ['Magic', 88],
        ['Bestiary', 90], ['Achievements', 92], ['Wait', 94], ['CustomSkills', 96], ['Character', 100]]) {
        const menuIcon = shape(xml[1], id);
        const box = menuIcon.match(/<shapeBounds[^>]+>/)[0];
        for (const axis of ['X', 'Y']) {
            const max = Number(box.match(new RegExp(`${axis}max="(-?\\d+)"`))[1]);
            const min = Number(box.match(new RegExp(`${axis}min="(-?\\d+)"`))[1]);
            assert.ok(max > min, `${name} has nonzero vector bounds`);
        }
        assert.ok((menuIcon.match(/type="(?:Straight|Curved)EdgeRecord"/g) || []).length > 10,
            `${name} contains imported vector paths`);
        const exports = [...xml[1].matchAll(/<item type="ExportAssetsTag"[^>]*>[\s\S]*?<\/names>\s*<\/item>/g)];
        assert.ok(exports.some(e => e[0].includes(`<item>${id + 1}</item>`) &&
            e[0].includes(`<item>Navbar${name}Icon</item>`)), `${name} sprite linkage is exported`);
        assert.ok(xml[1].includes(`spriteId="${id + 1}"`));
        if (id >= 88) {
            const colors = [...menuIcon.matchAll(/<color type="RGBA"[^>]*\/>/g)].map(m => m[0]);
            assert.ok(colors.length > 0);
            assert.ok(colors.every(color => ['red', 'green', 'blue', 'alpha'].every(channel => color.includes(`${channel}="255"`))),
                `${name} is opaque white vector geometry without frame backgrounds or invisible guide shapes`);
        }
    }
    assert.ok(xml[1].includes('NavbarWidget'), 'widget linkage exported');
    console.log('Compiled SWF verified: rectangular sidebar background, fixed-edge nine-slice grid, vector icon, linkages.');
} finally {
    assert.equal(path.dirname(path.resolve(temporary)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(temporary).startsWith('navbar-swf-test-'));
    fs.rmSync(temporary, { recursive: true, force: true });
}
