# AGENTS.md

Guidance for humans and coding agents working in this repository.

This repo builds the `crypto-finder-image` container image (see
[`Containerfile`](Containerfile)) through Konflux pipelines defined in
[`.tekton/`](.tekton).

## Build constraints

### The build is not hermetic, and the `hermetic` param must stay `"false"`

Konflux's `hermetic` parameter runs the build with network isolation. This
image cannot build that way today, because two steps reach the network at
image build time:

1. **Go module download** — [`Containerfile:11`](Containerfile) runs
   `go mod download` in the builder stage to fetch the `crypto-finder`
   dependencies.
2. **Opengrep release binary** — [`Containerfile:34-35`](Containerfile)
   copies and runs [`install-opengrep.sh`](install-opengrep.sh), which
   queries `https://api.github.com/repos/opengrep/opengrep/releases` and
   downloads the release binary from
   `https://github.com/opengrep/opengrep/releases/download/...`.

The `prefetch-dependencies` task cannot compensate for either one:
`prefetch-input` defaults to `""` in both build pipelines, so the task is a
no-op. Even with a populated `prefetch-input`, Cachi2 would cover the Go
modules but not an arbitrary release binary pulled from GitHub.

Setting `hermetic` to `"true"` on its own therefore does not make the build
hermetic — it makes it fail, or masks the real problem rather than fixing
it. This was attempted in
[PR #51](https://github.com/rh-pvsec/crypto-finder-image/pull/51), which was
closed unmerged for exactly this reason.

### Path forward

To make hermetic builds genuinely possible, in roughly this order:

1. Stop fetching the Opengrep binary over the network — vendor it, build it
   from source in a builder stage, or consume it from a trusted registry
   image rather than a GitHub release.
2. Populate `prefetch-input` (e.g. `gomod`) so `prefetch-dependencies`
   actually prefetches the Go dependencies, and drop the in-build
   `go mod download`.
3. Only then flip `hermetic` to `"true"`, in **both**
   [`.tekton/crypto-finder-image-pull-request.yaml`](.tekton/crypto-finder-image-pull-request.yaml)
   and
   [`.tekton/crypto-finder-image-push.yaml`](.tekton/crypto-finder-image-push.yaml).
   (`.tekton/crypto-finder-image-test.yaml` is the integration-test pipeline
   and declares no `hermetic` param.)

Related work: `install-opengrep.sh` can verify release signatures with
cosign, and [`Containerfile:33`](Containerfile) carries a
`TODO: cosign verification of binary`. Whatever replaces the current
download should address that TODO rather than inherit it.

### Enterprise Contract

Conforma/Enterprise Contract includes a `hermetic_build_task` rule, which is
the policy pressure behind requests to enable hermetic builds. Exception
status is not identical across the release pipelines this component feeds —
confirm the current state with the maintainers
(exd-guild-security@redhat.com) before assuming a non-hermetic build is
acceptable in a given pipeline. That topology is not recorded in this
repository.

## Working with the `.tekton` pipelines

The files in `.tekton/` are generated and managed by Konflux tooling and may
be rewritten when Konflux regenerates them. The inline comment above the
`hermetic` param is a convenience pointer; if a regeneration strips it,
re-add it. **This document is the durable record** — keep it accurate first.
