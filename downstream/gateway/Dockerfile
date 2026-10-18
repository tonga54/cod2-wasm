FROM node:22-alpine
WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci --omit=dev --ignore-scripts --no-audit --no-fund
COPY server.mjs updates.mjs maintenance.mjs classes.mjs classes.json ./
# A new named volume takes this directory's owner: the classes database.
RUN mkdir -p /data && chown node:node /data
ENV COD2_CLASSES_DB=/data/classes.sqlite
USER node
EXPOSE 8088/tcp
CMD ["node", "--disable-warning=ExperimentalWarning", "server.mjs"]
