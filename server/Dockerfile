FROM node:22.13.0-bookworm-slim AS dependencies
WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci --omit=dev

FROM node:22.13.0-bookworm-slim
ENV NODE_ENV=production MEETING_HOST=0.0.0.0 MEETING_DATA_DIR=/var/lib/meeting MEETING_TLS_CERT=/run/meeting-tls/cert.pem MEETING_TLS_KEY=/run/meeting-tls/key.pem
WORKDIR /app
COPY --from=dependencies --chown=node:node /app/node_modules ./node_modules
COPY --chown=node:node package.json package-lock.json ./
COPY --chown=node:node src ./src
USER node
EXPOSE 3443
CMD ["node", "src/index.mjs"]
