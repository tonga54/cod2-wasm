# meshoptimizer offline simplifier

`meshopt_simplifier.mjs` is the unmodified ES module from the npm
`meshoptimizer` 1.1.1 package, renamed from `.js` to `.mjs` so the offline Node
helper does not depend on a project package type. MIT license; see LICENSE.md.

Upstream: https://github.com/zeux/meshoptimizer
API: https://github.com/zeux/meshoptimizer/blob/v1.1/js/README.md#simplifier

Used only by `scripts/import-carentan-props.py` to simplify owner-provided
models while accounting for UV and normal error. It is not loaded by the game.
