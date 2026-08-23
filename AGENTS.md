# Codex Agent Instructions

This file is guidance for AI coding agents† such as OpenAI Codex when they work in this repository. It tells the agent how to run project checks and what operations require extra approval. It is not used by Arduino, the compiler, or the library at runtime.

## Arduino Build Tests

- Use `build-arduino-cli.bat` for Arduino compile checks in this repo.
- The wrapper discovers the local Arduino CLI install, Arduino packages, and user library paths. Those paths are usually outside the Codex workspace sandbox.
- When running compile-only Arduino build tests, request escalated permissions immediately instead of first attempting a sandboxed build.
- Use this command shape:
  `cmd /c build-arduino-cli.bat <sketch-path> <fqbn>`
- If no board is specified by the user, use `arduino:avr:nano` for Nano-targeted examples.
- This policy applies only to local compile/build verification. Uploads, package installs, core installs, library installs, deletes, and cleanup still require separate explicit approval.

## Arduino Include Resolution

- Arduino CLI is not a plain C++ compiler invocation. It discovers Arduino libraries and adds their `src` directories to the compiler include path.
- Use normal Arduino-style includes, such as `#include <AccelStepper.h>`, not pathful includes such as `#include <AccelStepper/src/AccelStepper.h>`.
- If an include fails, investigate Arduino library discovery and configuration before changing include paths in source files.

## Workspace Map

- This repository is an Arduino library for DCS-BIOS EasyMode.
- Public include entry point: `src/DcsBiosEasyMode.h`.
- Internal implementation headers live under `src/internal/`.
- Example sketches under `examples/` are used as compile-test coverage.
- Generated Arduino build output goes under `.arduino-build/` and should not be treated as source.

## Programming Style Expectations

- Prefer root-cause fixes over local workarounds.
- Avoid Helper Bloat. Prefer small code blocks be inserted when only used by a single caller.
- Be mindful of stack depth bloat.
- When using an architecture specific code block make an #if #else #end pre-processor block leveraging `#error`.

```cpp
#if defined(__AVR__)
#include <avr/wdt.h>
// AVR implementation
#elif defined(ESP32)
// ESP32 implementation
#else
#error Architecture Not Implemented. Write your port here.
#endif
```

In some cases, not having it implemented and falling through is ok. In such cases change `#error` to a `#warning`

```cpp
void reboot() {
#if defined(__AVR__)
    wdt_enable(WDTO_15MS);
    while (true) {}
#else
#warning EasyMode reboot is not implemented for this architecture.
#endif
}
```

## Development Expectations

- Preserve existing sketch compatibility unless the user explicitly asks for an API break.
- Design EasyMode APIs for non-programmers building hardware panels. Keep programming details hidden inside the classes, prefer Arduino-familiar terms such as `HIGH`, `LOW`, pins, degrees, and RPM, and avoid requiring users to understand C++ enum/class syntax for normal sketch use.
- EasyMode APIs are up for grabs while in development. Prefer the clearest beginner-facing sketch syntax over preserving temporary constructor argument order, and treat compatibility as negotiable until an API has been released or documented as stable.
- Do not edit example sketches unless the task requires it.
- For stepper changes, check both normal DCS-driven steppers and manual steppers when relevant.
- For public API changes, update or compile at least one representative example.

## Git Workflow

- When the user says "give me a commit", provide commit message text only: a concise summary line and a short descriptive body suitable for `git commit`.
- Do not run `git commit`, stage files, or modify Git history unless the user explicitly asks for that operation.

## Useful Compile Checks

- Nano-targeted active example:
  `cmd /c build-arduino-cli.bat examples\96_Spitfire_Altimeter_Barometer arduino:avr:nano`
- Mega/manual-stepper sanity check when stepper internals change:
  `cmd /c build-arduino-cli.bat examples\6_Mosquito_Blind_Panel arduino:avr:mega`

## Full Example Build Matrix

- Use `build-all-examples.bat <fqbn>` when validating broad compatibility across all example sketches.
- This script calls `build-arduino-cli.bat` for each example, so use the same escalated compile-only permission policy.
- Always pass the intended FQBN explicitly unless the task is specifically testing the default from `build-all-examples.cfg`.
- Use targeted example builds for normal iteration; use `build-all-examples.bat` for public API changes, release checks, or broad refactors.

## Release Packaging

- `release-local.bat` creates the Arduino library release zip under `dist/` by running `npm run release:zip`.
- Use `release-local.bat` only when validating packaging or preparing a local release artifact.
- `release-install-local.bat` runs `npm run release:install`, builds the zip, and installs it into the configured Arduino sketchbook libraries directory.
- Treat `release-install-local.bat` as a user-environment modification. Do not run it unless the user explicitly asks.
- Do not use release scripts as a substitute for compile checks; use `build-arduino-cli.bat` for normal validation.

† An agent is an AI coding assistant that can read the repository, edit files when allowed, and run local commands such as build or test scripts.
