# crypto-finder Builder based on:
# https://github.com/scanoss/crypto-finder/blob/main/Dockerfile
FROM  registry.access.redhat.com/ubi9/go-toolset:1.26.4-1782980183@sha256:977e77d5b7bdc1d3cce0c14d832245d765da4b6eaa83592594b5b522a8708aa7 AS builder
USER root

# Set working directory
WORKDIR /build

# Copy go mod files first for better layer caching
COPY crypto-finder/go.mod crypto-finder/go.sum ./
RUN go mod download

# Copy source code
COPY crypto-finder .

# Build arguments for version injection
ARG VERSION=dev
ARG GIT_COMMIT=unknown
ARG BUILD_DATE=unknown

# Build the binary with version info injected
RUN CGO_ENABLED=1 GOOS=linux go build --tags netgo \
    -ldflags="-w -s -X github.com/scanoss/crypto-finder/internal/version.Version=${VERSION} \
    -X github.com/scanoss/crypto-finder/internal/version.GitCommit=${GIT_COMMIT} \
    -X github.com/scanoss/crypto-finder/internal/version.BuildDate=${BUILD_DATE}" \
    -o crypto-finder \
    ./cmd/crypto-finder

# Final image based on UBI 9
FROM registry.access.redhat.com/ubi9/ubi-minimal:latest@sha256:932acc913841596b4bdb1dfb6280987b81c2d9f4d337d0a57c9a98fc749e10bd as final

# Install OpenGrep (minimum version 1.12.1)
# TODO: cosign verification of binary
COPY install-opengrep.sh /root
RUN /root/install-opengrep.sh -v v1.16.4 \
    && rm -f /root/install-opengrep.sh

# Add opengrep to PATH
ENV PATH="$PATH:/root/.opengrep/cli/latest"

COPY --from=builder /build/crypto-finder /usr/local/bin/crypto-finder

RUN mkdir -p /usr/local/rules

WORKDIR /workspace

LABEL name="crypto-scanner-image" \
    summary="Tool and rules to scan sources and detect cryptographic algorithms." \
    description="Tool and rules to scan sources and detect cryptographic algorithms." \
    maintainer="exd-guild-security@redhat.com" \
    vendor="Red Hat, Inc."

ENTRYPOINT [ "crypto-finder", \
    "scan", "--no-remote-rules", \
    "--rules-dir", "/usr/local/rules/crypto-rules", \
    "--rules-dir", "/usr/local/rules/rh-crypto-rules", \
    "--format", "cyclonedx", \
    "--timeout", "60m" \
]
