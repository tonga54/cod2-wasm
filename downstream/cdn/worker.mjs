// The Workers runtime treats each named export of the main module as an
// entrypoint, so this module exports only the handler.
import { createHandler } from './handler.mjs';

export default { fetch: createHandler() };
