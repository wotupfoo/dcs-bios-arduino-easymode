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
  - It always points `arduino-cli` at this workspace copy of the library.
  - If `%LocalAppData%\Arduino15\libraries` exists, it is also added so CLI builds can find Arduino IDE-managed libraries such as `Servo`.

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
build-all-examples.bat arduino:avr:uno
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
- The batch files also auto-detect `%LocalAppData%\Arduino15\libraries` so CLI builds can see libraries installed by the Arduino IDE.

## Releasing

There is no local `make_release` script in this repository, but there is now a
GitHub Actions workflow for building a curated Arduino-user ZIP:

- `.github/workflows/package-arduino-release.yml`
  - Runs on `workflow_dispatch`
  - Runs automatically when a GitHub release is published
  - Calls the same local npm packaging command used for manual release ZIP creation

- `package.json`
  - Provides local npm scripts for release packaging.

- `scripts/package-arduino-release.mjs`
  - Creates the curated Arduino-user ZIP in `dist/`.

Current manual workflow:

1. Bump the version number in `library.properties`.
2. Commit and push the release changes.
3. Run `npm run release:zip` to create the local Arduino-user ZIP in `dist/`.
4. If you want to update your local Arduino sketchbook copy from this repo, run `npm run release:install`.
5. Publish a GitHub release, or run the packaging workflow manually.
6. Download the generated ZIP artifact, or use the ZIP attached to the GitHub release.

Local packaging commands:

```bat
npm run release:zip
npm run release:zip:clean
npm run release:install
release-local.bat
release-install-local.bat
clean-local-temp.bat
clean-local-temp.bat --all
```

- `npm run release:zip`
  - Builds the Arduino-user release ZIP locally in `dist/`.
- `npm run release:zip:clean`
  - Removes the local release ZIP and the old unpacked staging folder in `dist/`.
- `npm run release:install`
  - Builds the Arduino-user release ZIP and installs it through `arduino-cli`, using the Arduino sketchbook path from `arduino-cli.yaml`.
- `release-local.bat`
  - Windows convenience wrapper for `npm run release:zip`.
- `release-install-local.bat`
  - Windows convenience wrapper for `npm run release:install`.
- `clean-local-temp.bat`
  - Removes repo-local `.tmp*` files and directories created during local testing.
- `clean-local-temp.bat --all`
  - Removes repo-local `.tmp*` files and directories and also deletes `dist/`.

These local commands are the developer-facing equivalent of the GitHub release
packaging workflow.

No `npm install` step is required for this packaging flow because the script
uses Node.js built-ins and platform zip tools only.

If the normal release ZIP is locked by another process on Windows, the local
packaging script falls back to creating a timestamped ZIP in `dist/` instead of
failing outright.

## Example Build Matrix

For a quick multi-board build check, run:

```bat
test-build-matrix.bat
```

This script reuses `build-all-examples.bat` and currently tests:

- Arduino Uno
- Arduino Mega 2560
- Arduino Nano (ATmega328P)
- STM32F103 Generic (`Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod`)

The STM32 test expects the Roger Clark `Arduino_STM32` core installed under the
Arduino sketchbook `hardware` folder. It does not use the official
`STMicroelectronics:stm32` core for this matrix.

If a required board core is not installed, that board is skipped.
If any tested board fails to build the examples, the script exits with an
overall failure code.

The Arduino-user ZIP intentionally includes:

- `src/`
- `examples/`
- `documentation/Example_Guide.md`
- `library.properties`
- `keywords.txt`
- `README.md`
- `LICENSE`

The ZIP intentionally excludes developer-only files such as:

- `.git*`
- `.github/`
- `.vscode/`
- `DEVELOPMENT.md`
- Arduino CLI batch files
- local Arduino CLI config files
