#!/usr/bin/env python3
"""Make a 60 Hz copy of a Hellnight (Europe) disc image.

The European release (SLES-01562) runs in 50 Hz PAL mode. This script copies
your Redump or Undub v1.1 dump and changes 9 bytes of the game's executable
so it runs in 60 Hz mode instead: the video-mode check is replaced with
"always NTSC" and one display setting is adjusted to match. These are the
same bytes as in the NTSC-converted dump the project was developed with.
The two changed disc sectors get fresh EDC/ECC, so the copy is a valid image.

Your original files are not changed. The copy is checked against known
digests, so Generate accepts it.

Usage:
    python make_60hz_disc.py "Hellnight (Europe).cue" [output-folder]

The output folder defaults to a new "<disc name> (60Hz)" folder next to the
source disc. Python 3.8 or newer, no extra packages.
"""

import hashlib
import shutil
import sys
from pathlib import Path

SECTOR = 2352
DATA = 24            # user data offset in a Mode 2 Form 1 sector
EXE_LBA = 24         # SLES_015.62 starts at this sector on every known dump
IMAGE_SIZE = 633786384

# offset in SLES_015.62 -> (50 Hz bytes, 60 Hz bytes)
PATCH = {
    0x19EB4: (bytes.fromhex("13"), bytes.fromhex("10")),
    0x23318: (bytes.fromhex("0880023c78a3428c"),   # lui/lw: read video mode
              bytes.fromhex("0000000000000224")),  # nop; li v0, 0 (NTSC)
}

# source MD5 -> (name, MD5 of the 60 Hz copy)
KNOWN = {
    "050d2848368af2ef945706bc73ef69ef":
        ("Redump", "01ce6367485ab848973be612df58b5b9"),
    "cb6f8e5a863920bf33d8cfe78dd684df":
        ("Undub v1.1", "c2dc2bdd0bbb373046405fa3b29bc5f5"),
}

_EDC = []
for _i in range(256):
    _e = _i
    for _ in range(8):
        _e = (_e >> 1) ^ (0xD8018001 if _e & 1 else 0)
    _EDC.append(_e)
_F = [0] * 256
_B = [0] * 256
for _i in range(256):
    _j = ((_i << 1) ^ (0x11D if _i & 0x80 else 0)) & 0xFF
    _F[_i] = _j
    _B[_i ^ _j] = _i


def _edc(data):
    e = 0
    for x in data:
        e = (e >> 8) ^ _EDC[(e ^ x) & 0xFF]
    return e


def _ecc(buf, major_count, minor_count, major_mult, minor_inc, out):
    size = major_count * minor_count
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        a = b = 0
        for _ in range(minor_count):
            t = buf[12 + index]
            index += minor_inc
            if index >= size:
                index -= size
            a ^= t
            b ^= t
            a = _F[a]
        a = _B[_F[a] ^ b]
        buf[out + major] = a
        buf[out + major + major_count] = a ^ b


def fix_sector(sector):
    """Recompute EDC and ECC of a Mode 2 Form 1 sector."""
    b = bytearray(sector)
    b[2072:2076] = _edc(b[16:2072]).to_bytes(4, "little")
    header = bytes(b[12:16])
    b[12:16] = b"\0\0\0\0"
    _ecc(b, 86, 24, 2, 86, 2076)
    _ecc(b, 52, 43, 86, 88, 2076 + 172)
    b[12:16] = header
    return bytes(b)


def md5_of(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def bin_from_cue(cue):
    for line in cue.read_text(errors="replace").splitlines():
        line = line.strip()
        if line.upper().startswith("FILE") and '"' in line:
            return cue.parent / line.split('"')[1]
    raise SystemExit(f"error: no FILE line in {cue}")


def main(argv):
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    src = Path(argv[1]).expanduser().resolve()
    src_bin = bin_from_cue(src) if src.suffix.lower() == ".cue" else src
    if not src_bin.is_file():
        raise SystemExit(f"error: disc image not found: {src_bin}")
    if src_bin.stat().st_size != IMAGE_SIZE:
        raise SystemExit("error: this is not a one-track Hellnight (Europe) "
                         "image (wrong size)")

    print(f"Checking {src_bin.name} ...")
    digest = md5_of(src_bin)
    if digest not in KNOWN:
        raise SystemExit(f"error: unknown dump (MD5 {digest}). Use the Redump "
                         "or Undub v1.1 dump of Hellnight (Europe). The "
                         "NTSC-converted dump already runs at 60 Hz.")
    name, want = KNOWN[digest]
    print(f"  {name} dump")

    stem = src_bin.stem + " (60Hz)"
    out_dir = (Path(argv[2]).expanduser().resolve() if len(argv) > 2
               else src_bin.parent.parent / stem)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_bin = out_dir / (stem + ".bin")
    out_cue = out_dir / (stem + ".cue")
    if out_bin.resolve() == src_bin:
        raise SystemExit("error: output would overwrite the source")

    print(f"Copying to {out_bin} ...")
    shutil.copyfile(src_bin, out_bin)
    with open(out_bin, "r+b") as f:
        touched = {}
        for offset, (old, new) in PATCH.items():
            for k in range(len(new)):
                o = offset + k
                lba = EXE_LBA + o // 2048
                if lba not in touched:
                    f.seek(lba * SECTOR)
                    touched[lba] = bytearray(f.read(SECTOR))
                pos = DATA + o % 2048
                if touched[lba][pos] != old[k]:
                    raise SystemExit("error: unexpected executable bytes")
                touched[lba][pos] = new[k]
        for lba, sector in touched.items():
            f.seek(lba * SECTOR)
            f.write(fix_sector(sector))
    out_cue.write_text(f'FILE "{out_bin.name}" BINARY\n'
                       "  TRACK 01 MODE2/2352\n"
                       "    INDEX 01 00:00:00\n")

    print("Verifying ...")
    got = md5_of(out_bin)
    if got != want:
        out_bin.unlink()
        raise SystemExit(f"error: result MD5 {got} != expected {want}")
    print(f"Done. Select this file in the setup wizard:\n  {out_cue}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
