# syntax=docker/dockerfile:1

# ---------------------------------------------------------------------------
# Stage 1: build the textparser CLI, ccat and the shared libraries.
# ---------------------------------------------------------------------------
FROM alpine:3.21 AS build

ARG TEXTPARSER_VERSION=""

RUN apk add --no-cache \
        bash \
        build-base \
        cmake \
        git \
        json-c-dev \
        ninja \
        pcre2-dev \
        pkgconf \
        python3

WORKDIR /src

COPY . .

# Pass an explicit TEXTPARSER_VERSION (e.g. from a git tag) when available.
# Otherwise the build falls back to `git describe --tags --abbrev=0`.
RUN if [ -n "$TEXTPARSER_VERSION" ]; then export TEXTPARSER_VERSION="$TEXTPARSER_VERSION"; fi \
    && BUILD_TESTS=OFF ./build.sh

# ---------------------------------------------------------------------------
# Stage 2: minimal runtime image with just the executables, shared libraries
# and their runtime dependencies.
# ---------------------------------------------------------------------------
FROM alpine:3.21 AS runtime

ARG TEXTPARSER_VERSION=""

LABEL org.opencontainers.image.title="textparser" \
      org.opencontainers.image.description="High-performance, extensible text parsing library with a CLI (textparser) and syntax highlighting utility (ccat)" \
      org.opencontainers.image.source="https://github.com/bokic/textparser" \
      org.opencontainers.image.licenses="MIT"

RUN apk add --no-cache \
        json-c \
        libpcre2-16 \
        libpcre2-32 \
        pcre2

COPY --from=build /src/bin/textparser /usr/local/bin/textparser
COPY --from=build /src/bin/ccat /usr/local/bin/ccat
COPY --from=build /src/bin/libtextparser.so* /usr/local/lib/
COPY --from=build /src/bin/libtextparser-json.so* /usr/local/lib/
COPY --from=build /src/bin/libtextparser_cfml.so* /usr/local/lib/
COPY --from=build /src/bin/libtextparser_css.so* /usr/local/lib/
COPY --from=build /src/bin/libtextparser_html.so* /usr/local/lib/
COPY --from=build /src/bin/libtextparser_php.so* /usr/local/lib/

# The executables are built with a build-tree RPATH pointing to the build
# output directory; LD_LIBRARY_PATH lets the loader find the textparser
# shared libraries at runtime.
ENV LD_LIBRARY_PATH=/usr/local/lib

ENTRYPOINT ["/usr/local/bin/textparser"]
