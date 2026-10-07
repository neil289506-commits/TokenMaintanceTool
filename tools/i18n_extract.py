#!/usr/bin/env python3
"""Extracts every translatable string (tr / QObject::tr / QT_TR_NOOP / QT_TRANSLATE_NOOP) from src/*.cpp and keeps
translations/TokenVault_en.ts in sync (existing translations are preserved).

  python3 tools/i18n_extract.py            # rewrite the .ts, list new strings as 'unfinished'
  python3 tools/i18n_extract.py --check    # exit 1 if a source string has no translation (used by ctest / CI)

Source language is Traditional Chinese (zh_TW). The runtime loader (src/i18n.cpp) reads the .ts directly, so no
lupdate / lrelease step is needed; the file is still a normal Qt Linguist .ts and can be edited with Linguist."""
import re, sys, glob, os, xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TS = os.path.join(ROOT, 'translations', 'TokenVault_en.ts')

CALL = re.compile(r'(?:\bQObject::tr|\bQCoreApplication::translate|\btr|\bQT_TR_NOOP|\bQT_TRANSLATE_NOOP)\s*\(')
STR = re.compile(r'"((?:[^"\\\n]|\\.)*)"')

def unescape(s):
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]
            out.append({'n': '\n', 't': '\t', '"': '"', '\\': '\\', "'": "'", 'r': '\r'}.get(n, n)); i += 2
        else:
            out.append(c); i += 1
    return ''.join(out)

def extract():
    found = {}
    for path in sorted(glob.glob(os.path.join(ROOT, 'src', '*.cpp')) + glob.glob(os.path.join(ROOT, 'src', '*.h'))):
        src = open(path, encoding='utf-8').read()
        for m in CALL.finditer(src):
            if src[m.start() - 1:m.start()] in ('.', '>') :            # obj.tr( / ptr->tr(  still a tr call; keep going
                pass
            i = m.end()
            # QT_TRANSLATE_NOOP("ctx", "text"): skip the context literal
            if 'TRANSLATE' in m.group(0) or 'translate' in m.group(0):
                sm = STR.match(src, src.index('"', i) if '"' in src[i:i + 40] else i)
                if not sm: continue
                i = sm.end()
                i = src.index(',', i) + 1
            parts, pos = [], i
            while True:
                while pos < len(src) and src[pos] in ' \t\r\n': pos += 1
                sm = STR.match(src, pos)
                if not sm: break
                parts.append(unescape(sm.group(1))); pos = sm.end()
            if parts:
                found.setdefault(''.join(parts), os.path.basename(path))
    return found

def load_ts():
    tr = {}
    if os.path.exists(TS):
        for msg in ET.parse(TS).getroot().iter('message'):
            s = msg.findtext('source') or ''
            t = msg.find('translation')
            tr[s] = (t.text or '') if t is not None else ''
    return tr

def write_ts(strings, tr):
    os.makedirs(os.path.dirname(TS), exist_ok=True)
    root = ET.Element('TS', version='2.1', language='en_US', sourcelanguage='zh_TW')
    ctx = ET.SubElement(root, 'context')
    ET.SubElement(ctx, 'name').text = 'TokenVault'
    for s in sorted(strings):
        m = ET.SubElement(ctx, 'message')
        ET.SubElement(m, 'source').text = s
        ET.SubElement(m, 'comment').text = strings[s]            # file the string comes from
        t = ET.SubElement(m, 'translation')
        if tr.get(s):
            t.text = tr[s]
        else:
            t.set('type', 'unfinished')
    ET.indent(root)
    with open(TS, 'wb') as f:
        f.write(b'<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n')
        f.write(ET.tostring(root, encoding='utf-8'))

if __name__ == '__main__':
    strings = extract()
    tr = load_ts()
    missing = sorted(s for s in strings if not tr.get(s))
    stale = sorted(s for s in tr if s not in strings and s)
    if '--check' in sys.argv:
        for s in missing: print('MISSING:', repr(s))
        print(f'{len(strings)} source strings, {len(missing)} missing, {len(stale)} stale')
        sys.exit(1 if missing else 0)
    write_ts(strings, tr)
    print(f'{len(strings)} strings; {len(missing)} still need an English translation; {len(stale)} stale entries dropped')
