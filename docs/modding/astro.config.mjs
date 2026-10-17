// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

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
          label: 'Hook Reference',
          items: [
            { slug: 'reference/hooks' },
            { slug: 'reference/hooks/lifecycle-world' },
            { slug: 'reference/hooks/glue' },
            { slug: 'reference/hooks/network' },
            { slug: 'reference/hooks/spells-tooltips' },
            { slug: 'reference/hooks/objects' },
            { slug: 'reference/hooks/input-chat' },
            { slug: 'reference/hooks/gameplay-ui' },
          ],
        },
        {
          label: 'API Reference',
          items: [
            { slug: 'reference/modcore' },
            { slug: 'reference/hooks-module' },
            { slug: 'reference/net' },
            { slug: 'reference/packet-io' },
            { slug: 'reference/dbc' },
            { slug: 'reference/visuals' },
            { slug: 'reference/api-export' },
            { slug: 'reference/configuration' },
            { slug: 'reference/entity-api' },
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
