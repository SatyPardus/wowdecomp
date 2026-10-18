import { hookDocId, moduleDocId } from './api-source.mjs';

/** @typedef {import('./api-source.mjs').ApiHook} ApiHook */
/** @typedef {import('./api-source.mjs').ApiModule} ApiModule */
/** @typedef {import('./api-source.mjs').ApiParam} ApiParam */
/** @typedef {import('./api-source.mjs').Api} Api */
/** @typedef {import('./api-notes.mjs').Note} Note */
/** @typedef {import('./api-notes.mjs').ModuleNote} ModuleNote */
/** @typedef {{ id: string, data: Record<string, unknown>, body: string }} Page */

const STATUS_BADGES = {
  'not-wired': { text: 'Not wired', variant: 'caution' },
  partial: { text: 'Partial', variant: 'caution' },
  stub: { text: 'Stub', variant: 'caution' },
  declared: { text: 'Typings only', variant: 'caution' },
};

const STATUS_ASIDES = {
  'not-wired': ['Not wired', 'The engine does not call this hook in the current source, so listeners never run.'],
  declared: ['Typings only', 'This type appears in the exported typings but has no Lua binding. The globals do not exist at runtime.'],
  stub: ['Prototype', 'This module is a scaffold and does not do its advertised work yet.'],
};

const cell = (/** @type {string} */ s) => String(s ?? '').replace(/\|/g, '\\|').replace(/\n/g, ' ');

/** @param {string} status */
function statusAside(status) {
  const a = /** @type {Record<string, string[]>} */ (STATUS_ASIDES)[status];
  return a ? `:::caution[${a[0]}]\n${a[1]}\n:::\n` : '';
}

/** @param {string | undefined} status */
function badge(status) {
  return status ? /** @type {any} */ (STATUS_BADGES)[status] : undefined;
}

/** @param {ApiParam[]} params @param {string} first */
function paramTable(params, first) {
  if (params.length === 0) return '';
  const rows = params.map(
    (p) => `| \`${p.name}${p.optional ? '?' : ''}\` | \`${cell(p.type)}\` | ${cell(p.description)} |`,
  );
  return [`| ${first} | Type | Description |`, '| --- | --- | --- |', ...rows, ''].join('\n');
}

/** @param {ApiHook} h */
function hookKind(h) {
  if (h.returnType === 'void') return ['Notification', 'The engine ignores the return value. Return nothing, because any non-nil value also stops later listeners.'];
  if (h.returnType === 'boolean?') return ['Consume', 'Return `true` to consume the event so the engine skips its default behavior. Return `nil` to let it continue.'];
  return ['Override', 'Return a value to replace the engine\'s own. Return `nil` to keep the stock behavior.'];
}

/** @param {ApiHook} h @param {Note | undefined} note @returns {Page} */
function hookPage(h, note) {
  const [kind, kindDoc] = hookKind(h);
  const args = h.params.map((p) => p.name).join(', ');
  const sig = `fun(${h.params.map((p) => `${p.name}${p.optional ? '?' : ''}: ${p.type}`).join(', ')}): ${h.returnType}`;
  const rows = [`| Category | ${h.category} |`, `| Type | ${kind} |`];
  if (note?.meta.source) rows.push(`| Fired from | ${cell(note.meta.source)} |`);

  const body = [
    `${h.description}.`,
    '',
    statusAside(note?.meta.status ?? ''),
    '| | |',
    '| --- | --- |',
    ...rows,
    '',
    '## Signature',
    '',
    '```lua',
    `-- ${sig}`,
    `ModCore.On("${h.name}", function(${args})`,
    '    -- ...',
    'end)',
    '```',
    '',
    h.params.length ? `## Parameters\n\n${paramTable(h.params, 'Name')}` : '',
    `## Returns\n\n\`${h.returnType}\`${h.returnDoc ? `: ${h.returnDoc}` : ''}\n\n${kindDoc}\n`,
    note?.body ? `## Notes\n\n${note.body}\n` : '',
  ].join('\n');

  return {
    id: hookDocId(h),
    data: {
      title: h.name,
      description: `${h.description}.`,
      sidebar: { badge: badge(note?.meta.status) },
    },
    body,
  };
}

/** @param {ApiModule} m @param {ModuleNote | undefined} note @returns {Page} */
function modulePage(m, note) {
  const props = [];
  if (note?.meta.access) props.push(`| Access | \`${cell(note.meta.access)}\` |`);
  if (m.parent) props.push(`| Extends | [\`${m.parent}\`](/${moduleDocId(m.parent)}/) |`);

  const prefix = note?.meta.access && !note.meta.access.includes('(') ? note.meta.access : m.name;
  const methods = m.methods.map((fn) => {
    const sig = `${prefix}.${fn.name}(${fn.params.map((p) => `${p.name}${p.optional ? '?' : ''}: ${p.type}`).join(', ')})`;
    return [
      `### ${fn.name}`,
      '',
      `\`${sig}\``,
      '',
      fn.description ? `${fn.description}.` : '',
      '',
      paramTable(fn.params, 'Parameter'),
      `**Returns** \`${fn.returnType}\``,
      '',
      note?.members.get(fn.name) ?? '',
      '',
    ].join('\n');
  });

  const body = [
    m.description ? `${m.description}.` : '',
    '',
    statusAside(note?.meta.status ?? ''),
    props.length ? ['| | |', '| --- | --- |', ...props, ''].join('\n') : '',
    note?.body ?? '',
    '',
    m.fields.length ? `## Fields\n\n${paramTable(m.fields, 'Field')}` : '',
    m.methods.length ? `## Methods\n\n${methods.join('\n')}` : '',
  ].join('\n');

  return {
    id: moduleDocId(m.name),
    data: {
      title: note?.meta.title ?? m.name,
      description: m.description,
      sidebar: { badge: badge(note?.meta.status) },
    },
    body,
  };
}

/** Starlight's aside transform does not run for loader-rendered Markdown, so emit its markup directly. */
function asides(/** @type {string} */ md) {
  return md.replace(/^:::(note|tip|caution|danger)(?:\[(.*?)\])?\n([\s\S]*?)\n:::$/gm, (_, type, title, inner) => {
    const label = title ?? type[0].toUpperCase() + type.slice(1);
    return [
      `<aside aria-label="${label}" class="starlight-aside starlight-aside--${type}">`,
      `<p class="starlight-aside__title" aria-hidden="true">${label}</p>`,
      '<div class="starlight-aside__content">',
      '',
      inner,
      '',
      '</div>',
      '</aside>',
    ].join('\n');
  });
}

/** @param {Api} api @param {{ hooks: Map<string, Note>, modules: Map<string, ModuleNote> }} notes */
export function buildPages(api, notes) {
  return [
    ...api.hooks.map((h) => hookPage(h, notes.hooks.get(h.name))),
    ...api.modules.map((m) => modulePage(m, notes.modules.get(m.name))),
  ].map((p) => ({ ...p, body: asides(p.body) }));
}

/** Names that have notes but no schema entry, for stale-note warnings. */
export function staleNotes(/** @type {Api} */ api, /** @type {{ hooks: Map<string, Note>, modules: Map<string, ModuleNote> }} */ notes) {
  const hooks = new Set(api.hooks.map((h) => h.name));
  const modules = new Set(api.modules.map((m) => m.name));
  return [
    ...[...notes.hooks.keys()].filter((n) => !hooks.has(n)).map((n) => `hook ${n}`),
    ...[...notes.modules.keys()].filter((n) => !modules.has(n)).map((n) => `module ${n}`),
  ];
}
