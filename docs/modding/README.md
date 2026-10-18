# wowdecomp ModCore documentation

Starlight (Astro) site documenting the Lua modding framework in `WoW/Source/ModCore` and `mods/`.

Requires Node.js 22.12 or newer.

```sh
npm install
npm run dev      # http://localhost:4321
npm run build    # static output in dist/
```

Pages live in `src/content/docs/`. The sidebar is defined in `astro.config.mjs`.

## Generated API reference

The hook and module pages under `reference/hooks/` and `reference/api/` are generated at build time by a custom docs loader (`src/lib/api-loader.mjs`). It reads `../../mods/api/wowdecomp_api.json`, the schema the client exports on start-up, and merges in the hand-written notes from `src/api-notes/`. The build fails if the JSON is missing. Set `MODCORE_API_JSON` to use a different file. See "Writing API notes" in the Building page for the notes format.
