# Wrangler for scripts/publish-cdn.py, which builds this image once per pinned version.
FROM node:22-bookworm-slim
ARG WRANGLER
RUN test -n "${WRANGLER}" \
 && npm install --global --no-audit --no-fund "wrangler@${WRANGLER}" \
 && npm cache clean --force
USER node
WORKDIR /work
ENTRYPOINT ["wrangler"]
