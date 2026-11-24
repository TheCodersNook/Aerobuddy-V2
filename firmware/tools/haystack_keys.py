#!/usr/bin/env python3
"""AeroBuddy v2 — OpenHaystack key generation + USB provisioning.

Generate NIST P-224 keypairs for the Find My beacon and provision the
public keys to the device over USB serial.

Usage:
  # Generate 672 keypairs (7 days at 15-min rotation), private keys to a 0600 file:
  python3 haystack_keys.py generate --count 672 --out ~/haystack-keys.json

  # Provision the public keys to the device:
  python3 haystack_keys.py provision --keys ~/haystack-keys.json --port /dev/tty.usbmodemXXXX

  # Provision public keys from another tool (one per line):
  python3 haystack_keys.py provision --hexfile keys.txt --port ...   # 56-char hex per line
  python3 haystack_keys.py provision --b64file keys.txt --port ...   # base64 per line

Requires: cryptography (generate only), pyserial (provision only).
  pip install cryptography pyserial

The keys file contains the PRIVATE keys — it is chmod 0600 on write.
Back it up somewhere safe. Never commit it, never share it: anyone
holding a private key can follow the beacon. Only PUBLIC keys ever go
to the device (HAYSTACK_PUT); the device never sees a private key.
"""

import argparse
import base64
import binascii
import hashlib
import json
import os
import sys
import time

KEY_BYTES = 28
MAX_KEYS = 1024  # firmware cap (HAYSTACK_MAX_KEYS)


# ------------------------------------------------------------------ keys
def generate_keypair():
    """One P-224 keypair. Returns dict of hex/base64 renderings.

    Private key: the 28-byte scalar d (big-endian). THE SECRET.
    Public key: the 28-byte X-coordinate of d*G (big-endian) — this is
    what the beacon advertises. Lookup index: SHA-256(public key),
    which finders file reports under and fetch tools query for.
    """
    from cryptography.hazmat.primitives.asymmetric import ec

    priv = ec.generate_private_key(ec.SECP224R1())
    d = priv.private_numbers().private_value.to_bytes(KEY_BYTES, "big")
    x = priv.public_key().public_numbers().x.to_bytes(KEY_BYTES, "big")
    return {
        "private_hex": d.hex(),
        "private_b64": base64.b64encode(d).decode(),
        "public_hex": x.hex(),
        "public_b64": base64.b64encode(x).decode(),
        "lookup_sha256_hex": hashlib.sha256(x).hexdigest(),
    }


def cmd_generate(args):
    if args.count < 1 or args.count > MAX_KEYS:
        print(f"error: --count must be 1..{MAX_KEYS}", file=sys.stderr)
        return 1
    keys = [generate_keypair() for _ in range(args.count)]
    doc = {
        "curve": "P-224",
        "device": "AeroBuddy v2",
        "created": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "count": len(keys),
        "rotation_minutes": 15,
        "warning": "PRIVATE KEYS — keep this file 0600, backed up offline, never shared.",
        "keys": keys,
    }
    with open(args.out, "w") as f:
        json.dump(doc, f, indent=2)
    os.chmod(args.out, 0o600)
    hours = args.count * 15 / 60
    cover = f"{hours:.1f} hours" if hours < 24 else f"{hours / 24:.1f} days"
    print(f"wrote {args.count} keypairs to {args.out} (0600)")
    print(f"covers {cover} at 15-min rotation "
          f"({args.count} x 15 min; device holds up to {MAX_KEYS})")
    print(f"first public key: {keys[0]['public_hex']}")
    print("next: provision the public keys with:")
    print(f"  python3 {sys.argv[0]} provision --keys {args.out} --port /dev/tty.usbmodemXXXX")
    return 0


# -------------------------------------------------------------- loading
def load_public_keys(args):
    """Return a list of 28-byte public keys from whichever input was given."""
    if args.keys:
        with open(args.keys) as f:
            doc = json.load(f)
        keys = []
        for i, k in enumerate(doc["keys"]):
            try:
                raw = bytes.fromhex(k["public_hex"])
            except (KeyError, ValueError):
                print(f"error: keys[{i}] has no valid public_hex", file=sys.stderr)
                return None
            if len(raw) != KEY_BYTES:
                print(f"error: keys[{i}] public key is not 28 bytes", file=sys.stderr)
                return None
            keys.append(raw)
        return keys
    if args.hexfile:
        return _load_text_keys(args.hexfile, "hex")
    if args.b64file:
        return _load_text_keys(args.b64file, "b64")
    print("error: provision needs --keys, --hexfile, or --b64file", file=sys.stderr)
    return None


def _load_text_keys(path, fmt):
    keys = []
    with open(path) as f:
        for ln, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            try:
                raw = bytes.fromhex(line) if fmt == "hex" else base64.b64decode(line)
            except (ValueError, binascii.Error):
                print(f"error: {path}:{ln}: not valid {fmt}", file=sys.stderr)
                return None
            if len(raw) != KEY_BYTES:
                print(f"error: {path}:{ln}: key is not 28 bytes", file=sys.stderr)
                return None
            keys.append(raw)
    return keys


# --------------------------------------------------------------- serial
class Device:
    """One USB-serial conversation with the AeroBuddy firmware."""

    def __init__(self, port, baud=115200):
        import serial  # pyserial; ImportError here has a helpful message below
        self.ser = serial.Serial(port, baud, timeout=0.2)

    def close(self):
        self.ser.close()

    def _readline(self):
        raw = self.ser.readline()
        return raw.decode("ascii", "replace").strip() if raw else ""

    def drain(self):
        """Discard the CSV auto-dump the device sends on USB connect.

        Ends when the dump's last line ("# Haystack: ...") arrives, after
        1 s of silence (board was already connected, no dump), or after
        10 s (a full 1000-line log dump takes ~2 s at 115200).
        """
        quiet_deadline = time.time() + 1.0
        hard_deadline = time.time() + 10.0
        while time.time() < hard_deadline:
            line = self._readline()
            if line:
                quiet_deadline = time.time() + 1.0
                if line.startswith("# Haystack:"):
                    return
            elif time.time() > quiet_deadline:
                return

    def command(self, line, until_prefix, timeout=3.0):
        """Send one line, return (found, lines). `found` is True when a
        reply line starting with `until_prefix` arrived in time."""
        self.ser.write((line + "\n").encode("ascii"))
        self.ser.flush()
        lines, end = [], time.time() + timeout
        while time.time() < end:
            text = self._readline()
            if text:
                lines.append(text)
                if text.startswith(until_prefix):
                    return True, lines
        return False, lines


def cmd_provision(args):
    keys = load_public_keys(args)
    if keys is None:
        return 1
    if not keys:
        print("error: no keys to provision", file=sys.stderr)
        return 1
    if args.start_index + len(keys) > MAX_KEYS:
        print(f"error: {args.start_index} + {len(keys)} keys exceeds the "
              f"device cap of {MAX_KEYS}", file=sys.stderr)
        return 1

    if args.dry_run:
        for j, k in enumerate(keys):
            print(f"HAYSTACK_PUT {args.start_index + j} {k.hex()}")
        print(f"# {len(keys)} PUT lines (dry run, nothing sent)")
        return 0

    try:
        dev = Device(args.port, args.baud)
    except ImportError:
        print("error: pyserial is not installed: pip install pyserial",
              file=sys.stderr)
        return 1
    except Exception as e:  # noqa: BLE001 — serial open errors vary by OS
        print(f"error: cannot open {args.port}: {e}", file=sys.stderr)
        print("hint: on macOS the XIAO shows as /dev/tty.usbmodemXXXX; "
              "check `ls /dev/tty.usbmodem*`", file=sys.stderr)
        return 1

    try:
        dev.drain()
        ok, lines = dev.command("HAYSTACK_COUNT", "keys=", timeout=3.0)
        if not ok:
            print("error: no reply from the device — is it powered, plugged "
                  "in, and running the new firmware?", file=sys.stderr)
            print("hint: open the port in a serial terminal at 115200 and "
                  "check that HELP lists HAYSTACK commands", file=sys.stderr)
            return 1
        have = 0
        for line in lines:
            if line.startswith("keys="):
                have = int(line.split("=", 1)[1])
        if args.start_index > have:
            print(f"error: device holds {have} keys; --start-index "
                  f"{args.start_index} would leave a gap (provision in "
                  f"order from 0, or resume at {have})", file=sys.stderr)
            return 1
        print(f"device holds {have} keys; provisioning {len(keys)} "
              f"from index {args.start_index} ...")
        for j, key in enumerate(keys):
            idx = args.start_index + j
            ok, lines = dev.command(f"HAYSTACK_PUT {idx} {key.hex()}",
                                    "OK ", timeout=3.0)
            err = next((l for l in lines if l.startswith("ERR")), None)
            if err or not ok:
                print(f"error: PUT {idx} failed: "
                      f"{err or 'no reply (timeout)'}", file=sys.stderr)
                print(f"stopped after {j}/{len(keys)} keys "
                      f"(re-run to resume at {idx})", file=sys.stderr)
                return 1
            if (j + 1) % 100 == 0 or j + 1 == len(keys):
                print(f"  ... {j + 1}/{len(keys)}")
        ok, lines = dev.command("HAYSTACK_COUNT", "key_index=", timeout=3.0)
        total = next((l for l in lines if l.startswith("keys=")), "?")
        adv = next((l for l in lines if l.startswith("advertising=")), "?")
        print(f"done. device now reports {total}, {adv}")
        print("the screen should show FIND MY ON (or send HAYSTACK_ON "
              "if the beacon was disabled)")
        return 0
    finally:
        dev.close()


# ------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(
        description="AeroBuddy v2 OpenHaystack keys: generate + provision.")
    sub = ap.add_subparsers(dest="cmd", required=True)

    g = sub.add_parser("generate", help="generate P-224 keypairs")
    g.add_argument("--count", type=int, default=672,
                   help="keypairs to generate (default 672 = 7 days)")
    g.add_argument("--out", required=True, help="output JSON file (0600)")

    p = sub.add_parser("provision", help="provision public keys to the device")
    p.add_argument("--keys", help="keys JSON from generate")
    p.add_argument("--hexfile", help="text file, one 56-char hex public key per line")
    p.add_argument("--b64file", help="text file, one base64 public key per line")
    p.add_argument("--port", required=True, help="serial port, e.g. /dev/tty.usbmodemXXXX")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--start-index", type=int, default=0,
                   help="first device index to write (resume a partial run)")
    p.add_argument("--dry-run", action="store_true",
                   help="print the PUT lines without opening serial")

    args = ap.parse_args()
    if args.cmd == "generate":
        try:
            return cmd_generate(args)
        except ImportError:
            print("error: cryptography is not installed: "
                  "pip install cryptography", file=sys.stderr)
            return 1
    return cmd_provision(args)


if __name__ == "__main__":
    sys.exit(main())
