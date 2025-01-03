# M8's Revolution Sound Tool
A CLI and library for introspecing, extracting and decoding wii Nintendoware sound files.

## Usage
`mrst list|extract|decode [options] file`

### Common options
`-o/--out` output file path for extract and decode operations. If not provided, a sensible name will be chosen (if one file is output, the same as the input with different file extension, otherwise a directory with the same name with ".d" appended to it)

### `mrst list` subcommand
Prints various information about the file

### `mrst extract` subcommand
Extracts files from archive (BRSAR or BRWAR)

- `--decode` additionally decodes subfiles while extracting
- `--extract-rwar` For BRSAR extraction, automatically extract any BRWARs encountered

### `mrst decode` subcommand
Decodes file into modern standard format. BRSTM/BRWAV files are converted to WAVE, BRBNK (and corresponding RWAR if applicable) files are converted to SoundFont 2 (sf2) and BRSEQ files are converted to MIDI.

## Support matrix
| File   | list | extract | decode |
| :---   | :--: | :-----: | :----: |
| BRSAR  | Y    | Y       | N/A    |
| BRWAR  | Y    | Y       | N/A    |
| BRWAV  | Y    | N/A     | Y      |
| BRSTM  | Y    | N/A     | Y      |
| BRBNK  | Y    | N/A     | Y*     |
| BRSEQ  | Y    | N/A     | Y      |
| BRWSD  | Y    | N/A     | N/A    |

\* decode subcommand on that file specifically doesn't work since external information is needed to create an SF2. Use `--decode` on the original BRSAR instead.

Although I tried to incorporate all the features of past decoder implementations, MIDI conversion is a work in progress and not all RSEQ behavior can be translated into MIDI.

## Recommended TODO
Eventually it would be nice if this tool supported re-converting from common formats to nintendoware (`mrst encode`) and re-packing archives (`mrst archive`).
