FROM node:22-alpine
WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci --omit=dev --ignore-scripts --no-audit --no-fund
COPY server.mjs ./
USER node
EXPOSE 8088/tcp
CMD ["node", "server.mjs"]
