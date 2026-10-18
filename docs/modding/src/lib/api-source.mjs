import fs from 'node:fs';
import path from 'node:path';

/** Schema written by the client's AutoExport (WoW/Source/ModCore/AutoExport.cpp). Override with MODCORE_API_JSON. */
export const apiJsonPath = path.resolve(
  process.env.MODCORE_API_JSON ?? path.join(process.cwd(), '..', '..', 'mods', 'api', 'wowdecomp_api.json'),
);

/**
 * @typedef {{ name: string, optional: boolean, type: string, description: string }} ApiParam
 * @typedef {{ name: string, description: string, returnType: string, params: ApiParam[] }} ApiMethod
 * @typedef {{ name: string, category: string, description: string, returnType: string, returnDoc: string, params: ApiParam[] }} ApiHook
 * @typedef {{ name: string, description: string, parent: string, fields: ApiParam[], methods: ApiMethod[] }} ApiModule
 * @typedef {{ name: string, opcode: number, opcodeHex: string, direction: string, description: string }} ApiOpcode
 * @typedef {{ version: string, hooks: ApiHook[], modules: ApiModule[], opcodes: ApiOpcode[] }} Api
 */

/** @returns {ApiParam} */
function normalizeParam(p) {
  const raw = String(p.name ?? '');
  return {
    name: raw.replace(/\?$/, ''),
    optional: raw.endsWith('?'),
    type: p.type ?? 'any',
    description: p.description ?? '',
  };
}

/** @type {{ mtimeMs: number, api: Api } | undefined} */
let cache;

/** @returns {Api} */
export function loadApi() {
  if (!fs.existsSync(apiJsonPath)) {
    throw new Error(
      `ModCore API schema not found at ${apiJsonPath}. Run the client once to export it, or set MODCORE_API_JSON.`,
    );
  }
  const { mtimeMs } = fs.statSync(apiJsonPath);
  if (cache?.mtimeMs === mtimeMs) return cache.api;

  const raw = JSON.parse(fs.readFileSync(apiJsonPath, 'utf8'));
  /** @type {Api} */
  const api = {
    version: raw.version ?? '',
    hooks: Object.entries(raw.hooks ?? {}).map(([name, h]) => ({
      name,
      category: h.category || 'General',
      description: h.description ?? '',
      returnType: h.returnType || 'void',
      returnDoc: h.returnDoc ?? '',
      params: (h.params ?? []).map(normalizeParam),
    })),
    modules: Object.entries(raw.modules ?? {}).map(([name, m]) => ({
      name,
      description: m.description ?? '',
      parent: m.parent ?? '',
      fields: (m.fields ?? []).map(normalizeParam),
      methods: (m.methods ?? []).map((/** @type {any} */ fn) => ({
        name: fn.name,
        description: fn.description ?? '',
        returnType: fn.returnType || 'void',
        params: (fn.params ?? []).map(normalizeParam),
      })),
    })),
    opcodes: raw.opcodes ?? [],
  };
  cache = { mtimeMs, api };
  return api;
}

/** Hook categories in schema order. */
export function hookCategories(/** @type {Api} */ api) {
  return [...new Set(api.hooks.map((h) => h.category))];
}

export function slug(/** @type {string} */ text) {
  return text.toLowerCase().replace(/[^a-z0-9]+/g, '-');
}

/** Hand-written notes (Markdown) merged into the generated pages. */
export const notesDir = path.join(process.cwd(), 'src', 'api-notes');

/** Exported hook categories are folded into fewer sidebar groups; unknown categories get their own group. */
const HOOK_GROUPS = [
  { slug: 'lifecycle-world', label: 'Lifecycle & world', categories: ['Lifecycle', 'World', 'Environment', 'Camera'] },
  { slug: 'glue', label: 'Glue screens', categories: ['Glue'] },
  { slug: 'network', label: 'Network', categories: ['Network'] },
  { slug: 'spells-tooltips', label: 'Spells & tooltips', categories: ['Spells', 'Tooltips'] },
  { slug: 'objects', label: 'Objects', categories: ['Objects'] },
  { slug: 'input-chat', label: 'Input & chat', categories: ['Input', 'Chat'] },
  {
    slug: 'gameplay-ui',
    label: 'Gameplay UI',
    categories: ['Loot', 'Merchant', 'Quests', 'ActionBar', 'Cooldowns', 'Sound', 'Items', 'Trade', 'Combat', 'UI'],
  },
];

export function hookGroupOf(/** @type {string} */ category) {
  return (
    HOOK_GROUPS.find((g) => g.categories.includes(category)) ?? { slug: slug(category), label: category, categories: [category] }
  );
}

/** Modules that mods call directly; everything else in the schema is an engine type. */
export const RUNTIME_MODULES = ['ModCore', 'Hooks', 'Net', 'DbcEngine', 'Visuals'];

const idPart = (/** @type {string} */ name) => name.toLowerCase().replace(/_/g, '-');

export const hookDocId = (/** @type {ApiHook} */ h) => `reference/hooks/${hookGroupOf(h.category).slug}/${idPart(h.name)}`;
export const moduleDocId = (/** @type {string} */ name) => `reference/api/${idPart(name)}`;

/** Sidebar groups for hooks, one per hook group, in schema order. */
export function hookSidebar(/** @type {Api} */ api) {
  /** @type {Map<string, { label: string, collapsed: boolean, items: { slug: string }[] }>} */
  const groups = new Map();
  for (const h of api.hooks) {
    const g = hookGroupOf(h.category);
    if (!groups.has(g.slug)) groups.set(g.slug, { label: g.label, collapsed: true, items: [] });
    groups.get(g.slug)?.items.push({ slug: hookDocId(h) });
  }
  return [...groups.values()];
}

export function moduleSidebar(/** @type {Api} */ api, /** @type {boolean} */ runtime) {
  return api.modules
    .filter((m) => RUNTIME_MODULES.includes(m.name) === runtime)
    .map((m) => ({ slug: moduleDocId(m.name) }));
}
