# AeroBuddy v2 — OpenHaystack Find My tracking

> **Status: implemented, compile-checked, NOT hardware-tested.** No Apple
> device has heard this firmware advertise yet. Bench verification (§6)
> is required before trusting it.

## 1. What this is

Your inhaler becomes a DIY AirTag. It broadcasts a rotating public key
over Bluetooth; any nearby iPhone, iPad, or Mac hears it, encrypts its
own GPS fix to that public key, and uploads the ciphertext to Apple.
You — and only you, holding the matching private keys — can decrypt
those reports and see where the inhaler is.

- **No MFi, no Apple approval, no companion app, no subscription.**
  This is the unofficial route (OpenHaystack), not Apple's certified
  Find My accessory program.
- **Locations do NOT appear in Apple's Find My app.** They appear in
  the fetch tool you run (§5): FindMy.py prints lat/lon;
  macless-haystack shows a map.
- **There is no GPS in the inhaler.** The location is the *finder's*
  location (the nearby iPhone's GPS fix), usually accurate to tens of
  meters — exactly how an AirTag works.

## 2. How it works (the protocol, plainly)

- One **P-224** elliptic-curve keypair per time slot. The **private
  key** `d` (28 bytes) is the secret: it stays on your computer and is
  used to decrypt reports. The **public key** is the curve point's
  28-byte X-coordinate — that is what the beacon broadcasts.
- The advertisement is a 31-byte BLE manufacturer packet
  (`ADV_NONCONN_IND`, non-connectable, nothing to pair to):

  ```
  1E FF 4C 00 12 19 [status] [key bytes 6..27] [key[0]>>6] [hint]
  │  │  └─┬─┘ │  │  │        │                   │           │
  │  │  Apple  │  │  │        │                   │           └─ 0x00, unused
  │  │  0x004C │  │  │        │                   └─ top 2 bits of key[0]
  │  │         │  │  │        └─ 22 bytes: the middle of the public key
  │  │         │  │  └─ status byte (0x00: battery unknown, not separated)
  │  │         │  └─ payload length (25)
  │  │         └─ Offline Finding type (0x12)
  │  └─ manufacturer-specific data
  └─ AD element length (30)
  ```

  The other 6 bytes of the key ride in the **BLE address** itself:
  `addr = key[0] | 0xC0, key[1], key[2], key[3], key[4], key[5]` (the
  top two bits of `key[0]` are forced to `0b11` because a random static
  address requires them; the original bits are recovered from the
  `key[0]>>6` payload byte). A finder rebuilds the full 28-byte key
  from address + payload, encrypts its location to it (ECIES), and
  files the report under `SHA-256(public key)`.
- **Rotation:** Apple rotates keys every 15 minutes; this firmware does
  the same. Keys are pre-generated on the computer (§4) and provisioned
  to the device, which cycles through them. After the list ends it
  wraps and reuses keys — re-provision for fresh rotation (§8).
- References: Heinrich, Stute, Kornhuber & Hollick, *"Who Can Find My
  Devices?"*, PoPETs/PETS 2021 (the SEEMOO lab paper behind
  OpenHaystack); `seemoo-lab/openhaystack`; `dchristl/macless-haystack`.

## 3. What the firmware does

- Keys live in LittleFS (`/haystack.bin`): an 8-byte header
  (`"HSTK"`, version, enabled flag, count) + up to 1024 × 28-byte
  **public** keys. Private keys are never on the device.
- The beacon **auto-starts at boot** once keys exist, and the main
  screen shows `FIND MY ON` while it advertises.
- Advertising: 1 s interval, +4 dBm, non-connectable. Key index follows
  wall-clock 15-minute slots when the clock is set (`SETTIME`), else
  15-minute slots since boot. Either way the fetch tools query *all*
  provisioned keys, so rotation alignment is for unlinkability, not
  correctness.
- Dose counting, e-ink UI, sleep, and CSV logging are unchanged.

Serial commands (115200, same terminal as the dose log):

| Command | What it does |
|---|---|
| `HAYSTACK_PUT <idx> <56 hex>` | Store one public key at index `idx` (provision in order from 0, no gaps) |
| `HAYSTACK_COUNT` | Keys stored, enabled, advertising, current index |
| `HAYSTACK_STATUS` | COUNT + current BLE address + full on-air payload (for scanner verification) |
| `HAYSTACK_ON` / `HAYSTACK_OFF` | Enable / disable the beacon (persisted) |
| `HAYSTACK_CLEAR` | Erase all keys, stop the beacon |

## 4. Keys: generate + provision

Prerequisites on your computer: Python 3, then
`pip install cryptography pyserial`.

**Step 1 — generate keys.** 672 keys = 7 days at 15-minute rotation
(96/day); up to 1024 fit on the device (~10.7 days).

```bash
python3 tools/haystack_keys.py generate --count 672 --out ~/haystack-keys.json
```

This writes one JSON file with the private + public key for every slot
plus each key's `SHA-256` lookup index. The file is `chmod 0600` and
**contains the private keys — back it up somewhere safe, never commit
it, never share it.** Anyone with a private key can follow the beacon.

**Step 2 — provision the public keys to the device** (USB cable, board
powered; find the port with `ls /dev/tty.usbmodem*` on macOS):

```bash
python3 tools/haystack_keys.py provision --keys ~/haystack-keys.json --port /dev/tty.usbmodemXXXX
```

The script sends one `HAYSTACK_PUT` per key, waits for the `OK` after
each, and verifies the count at the end. ~672 keys takes under a
minute. The beacon starts as soon as the first key lands (screen shows
`FIND MY ON`).

Keys from other tools work too: `--hexfile` takes a text file with one
56-char hex public key per line; `--b64file` takes one base64 public
key per line (the format macless-haystack shows in its UI).

**Re-provisioning:** send `HAYSTACK_CLEAR` over serial, then provision
again. (Overwriting a single key in place also works: `HAYSTACK_PUT`
with an existing index replaces it.)

## 5. Seeing the location

You need one fetch tool on your side. It logs into Apple with an Apple
ID, asks for reports filed under your keys' hashes, and decrypts them
with your private keys.

### FindMy.py (recommended — lightest path)

Pure Python, cross-platform, no Docker, no Mac-only tools, and recent
versions have a **built-in anisette generator** (no separate anisette
server needed). Repo: `github.com/malmeloo/FindMy.py`, install with
`pip install findmy`. Follow its `docs/getstarted/01-account.md` to
log in — **use a throwaway Apple ID with 2FA**, not your primary
account: this is unsanctioned use and a (small) ban risk is real.

Fetch one key (from the repo's `examples/`):

```bash
python3 fetch_reports.py <private_key_base64>
```

The private keys in base64 are in your `haystack-keys.json`
(`private_b64` per key). For a rotating beacon, fetch all of them:
`account.fetch_location([...])` accepts a list of `KeyPair` objects
(`KeyPair.from_b64(<private_b64>)`) and returns whichever keys have
reports — see FindMy.py's `docs/getstarted/02-fetching.md`, steps 2–3.
FindMy.py is alpha and its API may change; its docs are the authority.

### macless-haystack (nicer UI — a real map)

A self-hosted Docker stack (report endpoint + anisette server + web
UI) at `github.com/dchristl/macless-haystack`. Follow the repo's README
to bring it up, create the accessory there, and use its map to watch
locations. If you generated keys with §4 instead of in its UI, import
the private keys into its accessory store (its README documents the
format); FindMy.py's `examples/` also ships a
`macless_haystack_json_to_findmypy_json.py` converter for moving key
files between the two tools.

### OpenHaystack Mac app (not recommended for a new setup)

The original 2021 project (`seemoo-lab/openhaystack`): a macOS app plus
an Apple Mail plugin that borrows Mail's iCloud session to fetch
reports. It works, but it chains you to a Mac and Apple's Mail-plugin
hardening has broken it repeatedly since 2021. It is the historical
reference for the protocol, not the setup to start with today.

**What to expect:** first reports take minutes to hours — a passing
iPhone has to hear the beacon, and reports only exist where Apple
devices have been. In a house with iPhones around, expect the first
report within the hour. In a rural area with no Apple devices nearby,
there may be no reports at all until someone walks past.

## 6. Bench verification (do this before trusting it)

1. Provision keys (§4). `HAYSTACK_COUNT` should show the count; the
   screen shows `FIND MY ON`.
2. `HAYSTACK_STATUS`: note the BLE address and the `payload=` line.
3. With a BLE scanner (nRF Connect on a phone, LightBlue on a Mac),
   look for an advert whose manufacturer data starts `4C 00 12 19`.
   That is the Find My frame. (The displayed MAC looks reversed
   relative to the key — scanners show MSB first, the key bytes are in
   on-air order — so check the *payload*, not the MAC.)
4. Wait 15 minutes and check `HAYSTACK_STATUS` again: the key index,
   address, and payload must all have changed.
5. Fetch a report (§5) and confirm the lat/lon is near the device.
6. Measure battery current with the beacon on (meter in series, USB
   unplugged): expect roughly 20–30 µA total. If it is far higher, the
   beacon — not the dose code — is the suspect.

## 7. Battery

| | Light use | Typical | Heavy use |
|---|---|---|---|
| Beacon off (v4 sim) | 13.4 mo | 13.0 mo | 12.2 mo |
| **Beacon on** | **~4.5 mo** | **~4.5 mo** | **~4.4 mo** |

Math (110 mAh cell, 100 mAh usable — same basis as the v4 sim):
advertising at 1 s interval, +4 dBm, 31-byte payload on 3 channels
costs ~11 µA average in TX (3 × ~0.38 ms × ~10 mA), plus ~3 µA
SoftDevice idle and ~3 µA CPU wake overhead, rounded to **~20 µA**
average with margin. That is +0.48 mAh/day on top of the 0.253 mAh/day
typical baseline → 0.73 mAh/day → 100 / 0.73 ≈ 137 days. The beacon
dominates, so use level barely matters. Knobs: `HAYSTACK_ADV_MS`
2000 roughly halves the beacon draw (~6.5 months); 0 dBm saves
~4 µA. Assumptions: TX +4 dBm ≈ 10 mA (nRF52840 datasheet class),
sleep current as modeled in the v4 sim. **Measure it** — the sleep
current is still unmeasured on the real board, and it is the term the
months pivot on.

## 8. Honest limits

- **Not hardware-tested.** Compile-checked only. No Apple device has
  heard this firmware advertise; §6 is the proof that has to happen.
- **Unofficial.** Apple could change the protocol, throttle, or block
  unofficial beacons at any time. No guarantees, no support channel.
- **No Find My app, no Precision Finding** (no UWB chip), **no
  anti-stalking chirp.** Because the beacon is not paired to your
  Apple account, iPhones traveling with it may show "unknown
  accessory moving with you" alerts — including your own phone.
- **Location quality = the finder's GPS.** No reports without nearby
  Apple devices; first reports lag minutes to hours.
- **The private keys ARE the tracker.** Lose them and past reports are
  undecryptable. Leak them and anyone can follow the beacon. Keep the
  keys file `0600`, backed up offline, never in a repo or a chat.
- **Use a throwaway Apple ID** for fetching. A ban is unlikely but
  possible; isolate it from your real iCloud.
- **Keys repeat after the list wraps** (7 days at 672 keys). Rotation
  is the privacy feature — re-provision monthly, or provision the
  full 1024 (~10.7 days).
- **Use it to find your own inhaler.** Do not use it to track people.
