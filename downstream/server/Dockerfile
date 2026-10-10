FROM local/cod2-native-build:dev AS runtime-libraries
COPY out/cod2-native/cod2_lnxded /cod2_lnxded
# Copy the exact dynamic dependency closure from the compiler image. No SDK,
# headers, package cache or extra graphics drivers enter the runtime image.
RUN set -eu; mkdir -p /runtime/usr/local/bin; \
    cp /cod2_lnxded /runtime/usr/local/bin/cod2_lnxded; \
    ldd /cod2_lnxded > /dependencies; \
    ! grep -q 'not found' /dependencies; \
    awk '/=> \// {print $3} /^[[:space:]]*\// {print $1}' /dependencies \
      | while read -r library; do \
          case "$library" in /lib/*|/lib64/*) destination="/usr$library" ;; *) destination="$library" ;; esac; \
          mkdir -p "/runtime$(dirname "$destination")"; \
          cp -L "$library" "/runtime$destination"; \
        done

# Reuse the builder's distro Python for a small internal room supervisor.
# Copy its runtime closure, without pip, package caches, or compiler tools.
RUN set -eu; mkdir -p /runtime/usr/bin /runtime/usr/lib; \
    cp -L /usr/bin/python3 /runtime/usr/bin/python3; \
    cp -a /usr/lib/python3.10 /runtime/usr/lib/; \
    ldd /usr/bin/python3 /usr/lib/python3.10/lib-dynload/*.so > /python-dependencies; \
    awk '/=> \// {print $3} /^[[:space:]]*\/[^:]+[[:space:]]+\(/ {print $1}' /python-dependencies \
      | sort -u | while read -r library; do \
          case "$library" in /lib/*|/lib64/*) destination="/usr$library" ;; *) destination="$library" ;; esac; \
          mkdir -p "/runtime$(dirname "$destination")"; \
          cp -L "$library" "/runtime$destination"; \
        done; \
    rm -rf /runtime/usr/lib/python3.10/test /runtime/usr/lib/python3.10/idlelib \
           /runtime/usr/lib/python3.10/tkinter /runtime/usr/lib/python3.10/ensurepip

FROM ubuntu:22.04
COPY --from=runtime-libraries /runtime/ /
RUN mkdir -p /game/main /profile
COPY server.cfg /config/server.cfg
COPY downstream/server/browser_bots.gsc /config/browser_bots.gsc
# Include only the public hashes, never the private archives. A changed asset
# set then changes the image, so compose recreates the server's cached IWD list.
COPY site/wasm-game-data.json /config/browser-assets.json
COPY --chmod=755 downstream/server/entrypoint.sh /usr/local/bin/start-cod2
COPY downstream/server/rooms.py /usr/local/lib/cod2-rooms.py
WORKDIR /game
USER 1000:1000
EXPOSE 28960/udp
EXPOSE 8090/tcp
ENTRYPOINT ["/usr/local/bin/start-cod2"]
