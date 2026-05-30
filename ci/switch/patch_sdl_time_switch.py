#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

SWITCH_MARKER = "defined(DUSK_SWITCH_LIBNX_TOOLCHAIN)"
SWITCH_BLOCK = """        /* tm_gmtoff wasn't formally standardized until POSIX.1-2024, but practically it has been available on desktop
         * *nix platforms such as Linux/glibc, FreeBSD, OpenBSD, NetBSD, OSX/macOS, and others since the 1990s.
         *
         * Switch/libnx does not provide tm_gmtoff, so use the libc timezone fallback there.
         * The notable exception on traditional Unix is Solaris, where the timezone offset must still be retrieved in
         * the strictly POSIX.1-2008 compliant way.
         */
#if defined(SWITCH) || defined(__SWITCH__) || defined(DUSK_SWITCH_LIBNX_TOOLCHAIN)
        if (localTime) {
            tzset();
            dt->utc_offset = (int)_timezone;
        } else {
            dt->utc_offset = 0;
        }
#elif (_POSIX_VERSION >= 202405L) || (!defined(sun) && !defined(__sun))
        dt->utc_offset = (int)tm->tm_gmtoff;
#else
        if (localTime) {
            tzset();
            dt->utc_offset = (int)timezone;
        } else {
            dt->utc_offset = 0;
        }
#endif"""

OLD_BLOCK = """        /* tm_gmtoff wasn't formally standardized until POSIX.1-2024, but practically it has been available on desktop
         * *nix platforms such as Linux/glibc, FreeBSD, OpenBSD, NetBSD, OSX/macOS, and others since the 1990s.
         *
         * The notable exception is Solaris, where the timezone offset must still be retrieved in the strictly POSIX.1-2008
         * compliant way.
         */
#if (_POSIX_VERSION >= 202405L) || (!defined(sun) && !defined(__sun))
        dt->utc_offset = (int)tm->tm_gmtoff;
#else
        if (localTime) {
            tzset();
            dt->utc_offset = (int)timezone;
        } else {
            dt->utc_offset = 0;
        }
#endif"""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: patch_sdl_time_switch.py <SDL_systime.c>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    if not path.exists():
        print(f"patch_sdl_switch: SDL time file not found: {path}", file=sys.stderr)
        return 1

    text = path.read_text()
    if SWITCH_MARKER in text or "dt->utc_offset = (int)_timezone;" in text:
        print(f"patch_sdl_switch: SDL time fallback already patched")
        return 0

    if OLD_BLOCK not in text:
        print(f"patch_sdl_switch: failed to find SDL time fallback insertion point in {path}", file=sys.stderr)
        return 1

    path.write_text(text.replace(OLD_BLOCK, SWITCH_BLOCK, 1))
    print(f"patch_sdl_switch: patched {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
