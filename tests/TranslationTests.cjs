const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const parseJsonc = require('./Jsonc.cjs');
const [root, built] = process.argv.slice(2);
const source = path.join(root, 'src/ui/translations');
const catalogs = new Map();
for (const name of fs.readdirSync(source).filter(n => n.endsWith('.txt'))) {
    const original = fs.readFileSync(path.join(source, name), 'utf8');
    const encoded = fs.readFileSync(path.join(built, name));
    assert.equal(encoded[0], 0xFF, 'UTF-16 LE BOM');
    assert.equal(encoded[1], 0xFE, 'UTF-16 LE BOM');
    assert.equal(encoded.length % 2, 0, 'whole UTF-16 code units');
    const text = encoded.subarray(2).toString('utf16le');
    assert.equal(text.replace(/\r\n/g, '\n'), original.replace(/\r\n/g, '\n'));
    const entries = new Map();
    for (const line of text.trimEnd().split('\r\n')) {
        const pair = line.split('\t');
        assert.equal(pair.length, 2);
        assert.match(pair[0], /^\$SkyUINavbar_[A-Za-z0-9]+$/);
        assert.ok(pair[1].trim().length > 0);
        assert.ok(line.length <= 509, 'native importer line limit');
        assert.ok(!entries.has(pair[0]), 'unique keys');
        entries.set(...pair);
    }
    catalogs.set(name, entries);
}
const english = catalogs.get('SkyUINavbar_english.txt');
assert.ok(english);
for (const entries of catalogs.values()) {
    assert.deepEqual([...entries.keys()].sort(), [...english.keys()].sort(), 'complete language catalog');
    for (const [key, value] of english) {
        const tokens = s => (s.match(/\{[a-z]+\}/g) || []).sort();
        assert.deepEqual(tokens(entries.get(key)), tokens(value), 'binding placeholders survive translation');
    }
}
const config = parseJsonc(fs.readFileSync(path.join(root, 'config/SkyUINavbar.json'), 'utf8'));
assert.ok(!('schemaVersion' in config), 'no version metadata in user config');
for (const entry of config.menus) assert.deepEqual(Object.keys(entry).sort(), ['menu', 'showInNavbar', 'showNavbar']);
const widgetSource = fs.readFileSync(path.join(root, 'src/ui/actionscript/NavBar/NavbarWidget.as'), 'utf8');
for (const key of english.keys()) assert.ok(widgetSource.includes('"' + key + '"'), 'SWF owns ' + key);
assert.equal(english.size, 10, 'nine menu labels and hold hint; no removed tutorial strings');
for (const folder of ['src/skse', 'src/ui/actionscript/NavBar']) {
    for (const name of fs.readdirSync(path.join(root, folder)).filter(n => /\.(cpp|as)$/.test(n))) {
        const text = fs.readFileSync(path.join(root, folder, name), 'utf8');
        for (const match of text.matchAll(/"(\$SkyUINavbar_[A-Za-z0-9]+)"/g)) {
            if (match[1] !== '$SkyUINavbar_Key') assert.ok(english.has(match[1]), `missing ${match[1]}`);
        }
    }
}
console.log('Translation encoding, complete catalogs, placeholders and all UI keys verified.');
