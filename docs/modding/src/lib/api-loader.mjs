import { apiJsonPath, loadApi, notesDir } from './api-source.mjs';
import { loadNotes } from './api-notes.mjs';
import { buildPages, staleNotes } from './api-pages.mjs';

/** Ids written by the previous run, removed before regenerating. */
const generated = new Set();
/** Dev reloads re-evaluate this module, so the watcher guard has to live outside it. */
const WATCHING = Symbol.for('modcore-docs.watching');
const CONTEXT = Symbol.for('modcore-docs.context');
/** Serializes runs so overlapping file events cannot interleave store updates. */
let queue = Promise.resolve();

/** @param {any} ctx */
async function generate(ctx) {
  const { store, parseData, renderMarkdown, generateDigest, logger } = ctx;
  const api = loadApi();
  const notes = loadNotes();

  const stale = staleNotes(api, notes);
  if (stale.length) logger.warn(`API notes without a schema entry: ${stale.join(', ')}`);

  const pages = [];
  for (const page of buildPages(api, notes)) {
    const data = await parseData({ id: page.id, data: page.data });
    pages.push({
      id: page.id,
      data,
      body: page.body,
      digest: generateDigest(page.body + JSON.stringify(page.data)),
      rendered: await renderMarkdown(page.body),
    });
  }

  // Swap only after everything rendered, so a failed run never leaves the store half-updated.
  for (const id of generated) store.delete(id);
  generated.clear();
  for (const entry of pages) {
    store.set(entry);
    generated.add(entry.id);
  }
  logger.info(`Generated ${generated.size} API pages from ${apiJsonPath}`);
}

/** @param {any} ctx */
function enqueue(ctx) {
  queue = queue
    .catch(() => {})
    .then(() => generate(ctx))
    .catch((/** @type {Error} */ err) => ctx.logger.error(`API page generation failed: ${err.message}`));
  return queue;
}

/** Wraps Starlight's docs loader so the exported API schema and hand-written notes become docs entries. */
export function withApiPages(/** @type {any} */ inner) {
  return {
    ...inner,
    name: 'docs-with-api-pages',
    /** @param {any} ctx */
    load: async (ctx) => {
      await inner.load(ctx);
      // The first run must fail loudly; later runs from the watcher only log.
      const run = queue.catch(() => {}).then(() => generate(ctx));
      queue = run.catch(() => {});
      await run;

      const g = /** @type {any} */ (globalThis);
      g[CONTEXT] = ctx;
      if (ctx.watcher && !g[WATCHING]) {
        g[WATCHING] = true;
        ctx.watcher.add([apiJsonPath, notesDir]);
        ctx.watcher.on('change', (/** @type {string} */ file) => {
          if (file === apiJsonPath || file.startsWith(notesDir)) enqueue(g[CONTEXT]);
        });
      }
    },
  };
}
