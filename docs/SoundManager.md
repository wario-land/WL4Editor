# Sound Manager

The Sound Manager is available after a US (`AWAE`) ROM is loaded.

## Project folder

`Export All` writes to:

```text
<rom directory>/<rom name>_sound/
    soundmanifest.json
    README.txt
    psg.json
    songs/0001_SoundWarioWalk.s
    songs/0001_SoundWarioWalk.mid
    samples/0000_InstrumentUkuleleG2.wav
    soundfont/WL4.sf2
```

All addresses in the exported data are ROM file offsets. The GBA `0x08000000`
base is added only when writing pointers back into the ROM.

## Song source (`.s`)

The `.s` file is the authoritative song format. It contains:

- the song header,
- one byte-level MPlay track stream per track,
- labels for `GOTO`/`PATT`/`REPT` targets,
- metadata comments (`@ wl4-name`, `@ wl4-bank`, `@ wl4-ms`, ...).

`Import All` hashes each `.s` file against the manifest. Changed files are
assembled with the configured `arm-none-eabi-as/ld/objcopy` toolchain at the
address where the new `SoundSongChunk` will be written. The Song Table is then
patched directly; no sound list chunk is used.

MIDI is a lossy convenience export for MuseScore. It is not imported.

## Samples

Samples are exported twice:

- as editable 16-bit mono WAV files;
- inside the SoundFont 2 file.

`Import All` accepts changed WAVs and/or changed SF2 sample data. Samples are
written as `SoundSampleChunk`s. The chunk tail stores:

```text
W4S2 + sample id + sample name
```

so sample ids and names survive parser reloads. New SF2 sample entries are
added as new sample slots; the first save after adding one rewrites all sample
chunks with these trailers to keep ids stable.

## PSG sidecar

`psg.json` stores raw 12-byte PSG Program/sub-voice records:

```json
{
  "format": "WL4Editor.psg",
  "version": 1,
  "programs": [
    { "kind": "program", "bank": 1, "program": 0,
      "raw": "013c007f0200000000000f00" }
  ]
}
```

Edit `raw` in place to change pan/ADSR/type bytes. The importer compares it
against the manifest hash; changed records mark their Program and rebuild the
containing bank. `raw[4..7]` is preserved/rebuilt as the PSG wave pointer for
wave voices.

## SoundFont

The SF2 writer creates:

- one preset per sampled/key-split/drum Program;
- one instrument per Program;
- sample references, key ranges, loops, pan and ADSR;
- PSG Programs are omitted because SF2 cannot represent them.

The importer currently handles:

- sample PCM/loop replacement;
- simple-Program sample assignment, pan and ADSR;
- key-split/drum sample/pan/ADSR updates for layout-compatible zones;
- new sample slots.

SF2 presets whose bank/program index lands on an unused GBA program slot are
created there as simple sample Programs. Presets outside the bank data or on
already-used non-simple slots are skipped. ADSR conversion is approximate and
documented in `SoundEnvelope.cpp`.

## Save format

Sound data is saved through `ROMUtils::SaveFile`, not through `SaveLevel`.
Three chunk types are used:

- `SoundBankChunk` (`0x17`), with a `W4BK + bank id` trailer;
- `SoundSongChunk` (`0x18`);
- `SoundSampleChunk` (`0x19`), with a `W4S2` id/name trailer.

Bank chunks are rebuilt when a Program or one of its samples changes. Internal
bank pointers are rebased, sample pointers are rewritten, and all Song headers
using that bank are patched. Old chunks are invalidated. Banks that no
non-empty Song uses are not rewritten.

## Script API

```javascript
var folder = Sound.ExportSoundProject();            // default folder
var report = Sound.ImportSoundProject("", true);    // stage + save + reload
```

`ImportSoundProject(folder, false)` stages only. `ExportSoundProject(folder)`
accepts an explicit folder.

## Current limitations

- New SF2 presets are created only in unused, in-range GBA program slots.
- Key-split layouts that need more sub-voice slots than the bank contains are
  skipped with a warning.
- PSG Programs are preserved in Bank chunks but are not editable through SF2.
- ADSR mapping is approximate.
- `.mid` is export-only.
