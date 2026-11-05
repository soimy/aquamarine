# Independent scanout buffers for dual-DP 5K

This local patch adds an opt-in path on top of `imac5k-development` commit
`a3361e0772dd0e377dcaac0edbb53dcd5667a75f`:

```
AQ_DRM_TILED_OUTPUTS=1
AQ_DRM_TILED_COPY=1
```

The compositor still sees one 5120x2880 output. Each frame is copied on the
same GPU into two 2560x2880 scanout buffers. Both physical planes use source
coordinates (0, 0). The original shared-buffer path is unchanged when the
copy option is absent.

## Why

The local RTX 3090 / NVIDIA 615.78.08 / converted iMac panel setup has matching
native 2560x2880 timings at 59.98 Hz and a complete 2x1 TILE group. A previous
shared-buffer session exposed a single 5K output and a complete screenshot,
but the user physically observed only the blue left half. Two independent
outputs with separate native scanout buffers were physically confirmed to
show blue left and orange right on 2026-11-06.

This matches the class of nonzero horizontal scanout-offset failures tracked
by NVIDIA as bug 6265858, but does not prove that the driver's internal cause
is identical on this GPU.

## Implementation and constraints

- Three scanout buffers per head; allocation is restricted by the tiled
  group's shared format/modifier intersection. No forced LINEAR allocation.
- Pixel-aligned source rectangles, exact-size copies, no scaling. Ordinary
  full-buffer blits explicitly reset their texture coordinates after a crop.
- The experimental cropped blit calls `glFinish` before returning. The source
  and destination cannot be reused while the copy is outstanding. This is a
  conservative synchronous implementation; sustained-frame performance needs
  hardware measurement before replacing it with fence-driven scheduling.
- Both heads are submitted in the same atomic request. The existing two-head
  presentation barrier and scanout pinning remain in use.
- Failed and TEST_ONLY commits roll back acquired per-head swapchains.
  Buffers with scanout pins or retirement holds are never overwritten.
- The last copied source is tracked separately so ACTIVE-only DPMS restoration
  still works. VT restoration performs a complete group modeset as needed.
- Copy presentation does not advertise zero-copy. SDR, software cursors and
  the upstream experimental group's other restrictions still apply.
- Output detach shuts down private swapchains before EGL teardown. Existing
  device retirement retains scanout buffers if the seat is inactive.

No installed public header is changed by this patch. A consumer already
rebuilt against the exact base fork headers can be reused. A packaged stock
Hyprland binary is **not** compatible with the base fork's public layouts.

## Build and tests

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/opt/dual-dp-5k-copy" \
  -DCMAKE_INSTALL_LIBDIR=lib
cmake --build build -j8
ctest --test-dir build --output-on-failure -E simpleWindow
cmake --install build
```

The six existing noninteractive suites and the new `blit` crop-validation
suite passed. That does not replace physical scanout validation.

Local bounded-session scripts and per-run evidence are in
`~/.local/opt/dual-dp-5k-copy/tests/`. The original desktop is kept alive on its
own VT. Check `clients.json` and `mpv.log` before interpreting a visual test:
the first copy test had a test-harness Lua startup error and displayed no
test image. Its black background is not evidence of a failed image copy.

## Local physical validation, 2026-11-06

On the RTX 3090 and converted iMac panel described above:

- `unified-20261106-004426`: the user confirmed blue on the left, orange on
  the right, and a continuous green line across the panel. Hyprland exposed
  one 5120x2880 output at 59.982 Hz, scale 2.
- `unified-20261106-005808`: 74 control operations completed successfully.
  A floating window crossed the center seam, then returned to fullscreen.
  Both DPMS off/on cycles and switching away to the original session and
  back restored the whole image. The user confirmed no persistent black half.
- After restoration, CRTCs 205 and 397 used separate framebuffers 890 and 899,
  respectively, each at 2560x2880 with source coordinates (0, 0).
- Both successful sessions retired the DRM device with zero retained buffers,
  zero pins, and zero cleanup errors.

These results cover a temporary compositor session and static test content.
Cold startup, the first full Omarchy login, suspend/resume, cable hotplug and
sustained animation/game performance remain unverified. This patch changes
Hyprland scanout; it does not change the Linux text console's layout.

## Local deployment

The patched library is installed under
`/home/sym/.local/opt/dual-dp-5k-copy`. Its `bin/Hyprland` is the matching
0.56.2 consumer previously built against this fork's public headers and
`docs/hyprland-drm-shutdown.patch`; its RPATH now points to the new private
library directory. The original private installation is retained because the
consumer also refers to its installed assets. No packaged library is replaced.

`~/.local/bin/hyprland-dual-dp-5k` sets both opt-in variables and starts the
matched compositor. Its dynamic-load preflight falls back to packaged
Hyprland if the private binary cannot load. This is not a runtime crash
recovery mechanism; dependency upgrades can still require a matching rebuild.

The user systemd drop-in
`~/.config/systemd/user/wayland-wm@hyprland.desktop.service.d/60-dual-dp-5k.conf`
selects this wrapper through `start-hyprland` for the next login. The active
desktop has not been restarted. Monitor rules select the grouped preferred
5K mode, or the two native halves when using the packaged compositor.

To return subsequent logins to packaged Hyprland, run `dual-dp-5k-disable`,
then log out and back in. If the graphical session is unavailable, the same
command can be run from a logged-in TTY as the desktop user. Config backups
are under `~/.local/opt/dual-dp-5k-copy/deployment-backups/20261106-010212/`.

The normal user configuration passes offline parsing with `hl.timer` stubbed
only in the validation harness, avoiding the existing 0.56.2 verify-config
timer teardown bug. User timers are unchanged. The running stock compositor
also reports no configuration errors after reload.
