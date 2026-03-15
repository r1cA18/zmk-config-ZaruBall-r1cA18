# Dependency Pinning Policy

## Why this repo is pinned

This firmware is used on a daily-driver keyboard. Following upstream `main`
for ZMK, build workflows, and external modules makes the build non-repeatable
and can introduce regressions without any changes in this repository.

To keep the firmware reproducible, this repo pins:

- ZMK itself to the upstream release `v0.3.0`
- the GitHub Actions reusable workflow to `v0.3.0`
- external modules without tags to specific commit hashes

## Current pinned revisions

As of 2026-03-16:

- `zmk`: `ac7f75b8591d39aaf3b66b9d26f26c9ed921a009`
- `zmk-pmw3610-driver`: `a1a0f35ab6750c8fc72b84ea00add1e5fbaea1e4`
- `zmk-naginata`: `2544a5ad9df7eb7af1f0ae6382b8d29625a09576`
- `zmk-rgbled-widget`: `a3510c9de46de8b42c803286f4978466c6ea3916`
- GitHub Actions workflow: `zmkfirmware/zmk/.github/workflows/build-user-config.yml@ac7f75b8591d39aaf3b66b9d26f26c9ed921a009`

## Update rule

When updating dependencies:

1. Move one dependency at a time.
2. Build both halves.
3. Flash and test split BLE, encoder, trackball, and battery indication.
4. Commit the version bump separately from keymap changes.

If a module starts shipping stable tags, prefer the tag over a raw commit hash.
