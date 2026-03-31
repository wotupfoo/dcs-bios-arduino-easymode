# DCS-BIOS Easy Mode Development Notes

## Arduino CLI Helper Files

This repository includes a small set of Windows batch files and config files
to make example compilation easier with `arduino-cli`.

### Files

- `arduino-cli.yaml`
  - Local Arduino CLI config used by the batch files.
  - This usually points Arduino CLI at your Arduino sketchbook/libraries directory.
  - This file is machine-specific and may vary from one developer to another.
  - `build-all-examples.bat` can also read an optional `fqbn:` entry from this file.

- `arduino-cli.yaml.example`
  - Example template for `arduino-cli.yaml`.
  - Copy this to `arduino-cli.yaml` and adjust it if your Arduino sketchbook is in a different location.

- `build-all-examples.bat`
  - Recursively scans the `examples` folder.
  - For each example, it looks for a sketch whose `.ino` file matches its folder name.
  - It calls `build-arduino-cli.bat` once for each matching example.

- `build-all-examples.cfg`
  - Repository-level default config for `build-all-examples.bat`.
  - Currently used to define the default FQBN without editing the batch file.

- `build-arduino-cli.bat`
  - Compiles one sketch folder with `arduino-cli compile`.
  - Argument 1 is the sketch directory.
  - Argument 2 is an optional FQBN.
  - If no FQBN is provided, it falls back to `arduino:avr:uno`.

- `upload-arduino-cli.bat`
  - Uploads a single sketch folder using `arduino-cli`.
  - Uses `arduino-cli.yaml` for CLI configuration.

- `refresh-arduino-cli.bat (build+upload)`
  - Compiles and then uploads a single sketch folder using `arduino-cli`.
  - Uses `arduino-cli.yaml` for CLI configuration.

### FQBN Selection Order

When running `build-all-examples.bat`, the board is selected in this order:

1. Command-line argument
2. `fqbn:` entry from the file pointed to by `%ARDUINO_CONFIG_FILE%`
3. `build-all-examples.cfg`
4. Hardcoded fallback: `arduino:avr:uno`

Example:

```bat
build-all-examples.bat
build-all-examples.bat arduino:avr:mega
```

Default config file contents:

```ini
FQBN=arduino:avr:uno
```

Optional `arduino-cli.yaml` entry:

```yaml
fqbn: "arduino:avr:uno"
```

### Why There Are Two Config Files

- `arduino-cli.yaml` configures Arduino CLI itself, such as where your Arduino sketchbook lives.
- `build-all-examples.cfg` configures this repository's batch workflow, such as which board should be used by default for example builds.

## Releasing

There is currently no automated `make_release` script or release-packaging
mechanism in this repository.

Current manual workflow:

1. Bump the version number in `library.properties`.
2. Commit the release changes.
3. Create the release ZIP manually from the library contents you want to publish.
4. Upload that ZIP to GitHub as part of the release.
