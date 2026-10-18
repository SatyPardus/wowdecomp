import fs from 'node:fs';
import path from 'node:path';
import { notesDir } from './api-source.mjs';

const META_KEYS = ['status', 'source', 'access', 'title'];

/**
 * @typedef {{ status?: string, source?: string, access?: string, title?: string }} NoteMeta
 * @typedef {{ meta: NoteMeta, body: string }} Note
 * @typedef {{ meta: NoteMeta, body: string, members: Map<string, string> }} ModuleNote
 */

const normalize = (/** @type {string} */ t) => t.replace(/\r\n/g, '\n');

/** Leading `key: value` lines (whitelisted keys) are metadata; the rest is the body. */
function parseMeta(/** @type {string} */ text) {
  const lines = normalize(text).split('\n');
  /** @type {NoteMeta} */
  const meta = {};
  let i = 0;
  while (i < lines.length && lines[i].trim() === '') i++;
  while (i < lines.length) {
    const m = /^([a-z]+):\s*(.+)$/.exec(lines[i]);
    if (!m || !META_KEYS.includes(m[1])) break;
    /** @type {any} */ (meta)[m[1]] = m[2].trim();
    i++;
  }
  return { meta, body: lines.slice(i).join('\n').trim() };
}

/** Splits on heading lines recognized by `match` (returns the section name), ignoring code fences. */
function split(/** @type {string} */ text, /** @type {(line: string) => string | null} */ match) {
  /** @type {string[]} */
  const intro = [];
  /** @type {Map<string, string[]>} */
  const sections = new Map();
  /** @type {string[]} */
  let target = intro;
  let fenced = false;
  for (const line of normalize(text).split('\n')) {
    if (line.startsWith('```')) fenced = !fenced;
    const name = fenced ? null : match(line);
    if (name) {
      target = [];
      sections.set(name, target);
    } else {
      target.push(line);
    }
  }
  return {
    intro: intro.join('\n').trim(),
    sections: new Map([...sections].map(([k, v]) => [k, v.join('\n').trim()])),
  };
}

const h2 = (/** @type {string} */ line) => (line.startsWith('## ') ? line.slice(3).trim() : null);
const member = (/** @type {string} */ line) => (line.startsWith('## @') ? line.slice(4).trim() : null);

function read(/** @type {string} */ dir) {
  const full = path.join(notesDir, dir);
  if (!fs.existsSync(full)) return [];
  return fs
    .readdirSync(full)
    .filter((f) => f.endsWith('.md'))
    .map((f) => ({ name: path.basename(f, '.md'), text: fs.readFileSync(path.join(full, f), 'utf8') }));
}

/** @returns {{ hooks: Map<string, Note>, modules: Map<string, ModuleNote> }} */
export function loadNotes() {
  /** @type {Map<string, Note>} */
  const hooks = new Map();
  /** @type {Map<string, ModuleNote>} */
  const modules = new Map();

  // hooks/*.md: one `## HookName` section per hook
  for (const file of read('hooks')) {
    for (const [name, text] of split(file.text, h2).sections) hooks.set(name, parseMeta(text));
  }

  for (const file of read('modules')) {
    if (file.name === 'entities') {
      // entities.md: one `## TypeName` section per type
      for (const [name, text] of split(file.text, h2).sections) {
        modules.set(name, { ...parseMeta(text), members: new Map() });
      }
    } else {
      // <Module>.md: metadata + overview, then `## @Method` sections
      const { intro, sections } = split(file.text, member);
      modules.set(file.name, { ...parseMeta(intro), members: sections });
    }
  }
  return { hooks, modules };
}
