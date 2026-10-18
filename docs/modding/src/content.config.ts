import { defineCollection } from 'astro:content';
import { docsLoader } from '@astrojs/starlight/loaders';
import { docsSchema } from '@astrojs/starlight/schema';
import { withApiPages } from './lib/api-loader.mjs';

export const collections = {
  docs: defineCollection({ loader: withApiPages(docsLoader()), schema: docsSchema() }),
};
