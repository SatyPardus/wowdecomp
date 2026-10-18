// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import { hookSidebar, loadApi, moduleSidebar } from './src/lib/api-source.mjs';

const api = loadApi();

export default defineConfig({
  markdown: { gfm: true },
  integrations: [
    starlight({
      title: 'wowdecomp ModCore',
      description:
        'Reference and guides for the wowdecomp Lua modding framework (ModCore).',
      lastUpdated: false,
      sidebar: [
        {
          label: 'Getting Started',
          items: [
            { slug: 'getting-started/overview' },
            { slug: 'getting-started/directory-layout' },
            { slug: 'getting-started/first-mod' },
          ],
        },
        {
          label: 'Guides',
          items: [
            { slug: 'guides/hooks' },
            { slug: 'guides/recipes' },
            { slug: 'guides/custom-packets' },
            { slug: 'guides/virtual-dbc' },
            { slug: 'guides/hot-reload' },
            { slug: 'guides/ide-setup' },
          ],
        },
        {
          label: 'Reference',
          items: [
            {
              label: 'Hooks',
              items: [{ slug: 'reference/hooks', label: 'Overview' }, ...hookSidebar(api)],
            },
            {
              label: 'Runtime API',
              items: [
                ...moduleSidebar(api, true),
                { slug: 'reference/packet-io' },
                { slug: 'reference/api-export' },
                { slug: 'reference/configuration' },
              ],
            },
            { label: 'Engine types', collapsed: true, items: moduleSidebar(api, false) },
          ],
        },
        {
          label: 'Internals',
          items: [
            { slug: 'internals/architecture' },
            { slug: 'internals/wire-protocol' },
            { slug: 'internals/bytecode-cache' },
            { slug: 'internals/building' },
            { slug: 'internals/msdf-font-backend' },
          ],
        },
        { slug: 'known-issues' },
      ],
    }),
  ],
});
