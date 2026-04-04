# DCS-BIOS Easy Mode Beginner Guide

This guide is written for builders who may have little or no software experience.

This hobby is about making physically accurate and functional reproductions of aircraft cockpits to use inside [flight simulator](https://en.wikipedia.org/wiki/Flight_simulator) environments, [DCS World](https://www.digitalcombatsimulator.com/en/downloads/world/) being the simulator in this guide.

Leveraging the embedded electronics hobby ecosystem, switches, knobs, LEDs, and other input devices can control the simulated cockpit inside the game.

Likewise, things that move in the simulator, like gauge needles, can be made into functional reproductions of the real aircraft instruments using commonly available and inexpensive RC servo motors and tiny stepper motors like the ones often used to move needles in automotive dashboards.

The software pieces each do a different job:

- DCS World is the flight simulator.
- DCS-BIOS ([Basic Input/Output System](https://en.wikipedia.org/wiki/BIOS)) is the bridge that exposes cockpit data and accepts cockpit commands.
- DCS-BIOS Easy Mode makes the Arduino code much simpler and easier to copy, paste, and understand.
- Bort-EasyMode is a Windows companion app and live reference tool that helps the hobbyist find the information needed to implement a particular simulated item, like a gauge (output) or switch (input), in an Arduino application ("sketch") that is built and uploaded to an Arduino development board.

DCS-BIOS and this user-friendly version, DCS-BIOS Easy Mode, fuse the flight simulation hobby with the DIY electronics and Arduino software hobby to help create a [simulation cockpit](https://en.wikipedia.org/wiki/Simulation_cockpit).

Two words appear a lot in Arduino projects:

- Arduino: a small microcontroller board that can read switches, drive LEDs, and control small motors or servos.
- Sketch: the Arduino name for the program you write and upload to the board.

The goal is simple:

1. Install the software you need.
2. Open a blank Arduino sketch.
3. Find the right code snippet in BORT (The DCS-BIOS Reference Tool).
4. Copy and paste it into the blank sketch.
5. Make only a few small changes for your own hardware.

The examples in this guide are arranged as a learning path:

- Start with a very simple sketch.
- Move on to a one-way gauge.
- Then a centered-zero gauge.
- Then a more advanced derived gauge.
- And finally full aircraft panel examples.

If some of these words are new to you, that is normal. The point of this guide is to explain the workflow in the most accessible language possible.

This document is also intended to become a release handout later, so photo placeholders have been included where useful.

## What These Tools Do

Before starting, it helps to know what each tool is for.

### Arduino IDE (Integrated Development Environment)

This is the program used to open, edit, and upload Arduino sketches to your board.

### DCS-BIOS Skunkworks

This is the part that reads information from DCS World and makes that information available to external hardware projects.

Examples:

- altitude
- heading
- bank angle
- switch positions
- warning lights

### DCS-BIOS Easy Mode

This library sits on top of DCS-BIOS and tries to make the code read more like real instrument setup.

The DCS-BIOS Easy Mode library used in this guide comes from:

`https://github.com/WotUpFoo/dcs-bios-arduino-easymode`

Instead of thinking in low-level pulse widths or motor internals, the idea is to think in terms such as:

- minimum angle
- maximum angle
- trim
- clockwise or counter-clockwise
- zero at one end or zero in the middle

### Bort-EasyMode

Bort-EasyMode is the Easy Mode version of Bort, the DCS-BIOS reference and code-snippet tool.

You use Bort-EasyMode to:

- Look up the telemetry name you want.
- See live values.
- Choose a code snippet for your hardware.
- Copy that snippet into Arduino IDE.

## What You Need Installed

For a first Easy Mode project, install these five things:

1. [DCS World](https://www.digitalcombatsimulator.com/en/downloads/world/)
2. [Arduino IDE](https://www.arduino.cc/en/software/)
3. [DCS-BIOS Easy Mode Arduino library](https://github.com/wotupfoo/dcs-bios-arduino-easymode/releases)
4. [Bort-EasyMode](https://github.com/wotupfoo/Bort-EasyMode/releases)
5. [DCS-BIOS Skunkworks](https://github.com/DCS-Skunkworks/dcs-bios/releases)

## Suggested Beginner Workflow

If you are new, follow this order:

1. Install DCS World.
2. Install Arduino IDE.
3. Install the Arduino libraries.
4. Install Bort-EasyMode.
5. Install DCS-BIOS Skunkworks so DCS can export telemetry.
6. Open the blank Easy Mode sketch.
7. Open Bort-EasyMode and find a telemetry item.
8. Copy one snippet.
9. Paste it into the blank sketch.
10. Change only pins and any obvious instrument settings.
11. Upload to the Arduino board.

## Step 1: Install DCS World

Install DCS World the normal way for your computer:

https://www.digitalcombatsimulator.com/en/downloads/world/

If DCS World is not installed yet, it is better to do that first so the rest of the toolchain has a real simulator to connect to later.

## Step 2: Install Arduino IDE

Install Arduino IDE the normal way for your computer:

https://www.arduino.cc/en/software/

When it opens for the first time:

1. Connect your Arduino by USB.
2. Choose your board from the `Tools` menu.
3. Choose the correct COM port from the `Tools` menu.

If upload fails later, the most common cause is the wrong board or wrong COM port being selected.

Photo placeholder:

- Add screenshot of Arduino IDE with board and COM port selected.

## Step 3: Install The Arduino Libraries

You need the Arduino side of DCS-BIOS and the Easy Mode library available inside Arduino IDE.

The Easy Mode library for this guide comes from:

`https://github.com/wotupfoo/dcs-bios-arduino-easymode/releases`

For a beginner, the easiest mental model is:

- one library handles DCS-BIOS communication
- one library adds the Easy Mode shortcuts and examples

If these libraries are provided as ZIP files:

1. Open Arduino IDE
2. Choose `Sketch > Include Library > Add .ZIP Library...`
3. Add the DCS-BIOS Arduino library ZIP
4. Add the DCS-BIOS Easy Mode library ZIP

After installation, Arduino IDE should be able to open the example sketches from the `File > Examples` menu.

Photo placeholder:

- Add screenshot of `Add .ZIP Library...`

## Step 4: Install Bort-EasyMode

Install the DCS-BIOS Easy Mode version of Bort from:

https://github.com/wotupfoo/Bort-EasyMode/releases

Then run Bort-EasyMode.

When Bort-EasyMode first opens, it may need to be pointed at the DCS-BIOS JSON folder.

If that happens:

1. Open the `Menu`
2. Choose `Select dcs-bios location`
3. Point it to the DCS-BIOS JSON folder

Bort-EasyMode is mainly used as a lookup and copy-paste tool. Think of it as the parts catalog and code generator for the project.

Photo placeholder:

- Add screenshot of Bort-EasyMode showing a selected telemetry item and its code snippets.

## Step 5: Install DCS-BIOS Skunkworks

DCS-BIOS Skunkworks is what makes DCS telemetry available outside the simulator.

In simple terms:

- DCS World runs the aircraft.
- DCS-BIOS reads the aircraft data.
- Your Arduino sketch listens to that data.

The exact installer and screens may change over time, so if the current Skunkworks release looks a little different, that is normal.

What matters is that after installation:

- DCS-BIOS is installed into your `Saved Games` area
- DCS is allowed to export data
- live telemetry is available while DCS is running

For Bort-EasyMode, the important JSON files are normally found in a folder like:

`Saved Games\DCS\Scripts\DCS-BIOS\doc\json`

Photo placeholder:

- Add screenshot of the DCS-BIOS folder in Saved Games.

## Step 5: Open The Blank Starting Sketch

A blank starting sketch has been included for beginners:

`examples/0_DefaultSerial/0_DefaultSerial.ino`

This is the recommended first file to open when building from a Bort-EasyMode snippet.

It already contains:

- the required `#define DCSBIOS_DEFAULT_SERIAL`
- the correct Easy Mode include
- `DcsBios::setup();`
- `DcsBios::loop();`
- comments showing where copied lines should be pasted

If you are unsure where copied code belongs, start with this file.

## Step 6: Start DCS And Open Bort-EasyMode

Start DCS World and load into a flight where the instrument you want can be seen working.

Then open Bort-EasyMode and:

1. Find the aircraft or CommonData item you want
2. Search for the gauge or control
3. Look at the code snippets shown for that output

Choose the snippet that matches your hardware.

Examples:

- SG90 servo: choose the `DcsBios::EasyMode::Servo_SG90` snippet
- generic servo: choose the `DcsBios::EasyMode::Servo` snippet
- generic stepper: choose a generic stepper snippet
- 28BYJ-48 stepper: choose a `28BYJ48` snippet

## Step 7: Copy And Paste From Bort

This is the easiest beginner method.

### What To Copy

From Bort-EasyMode, copy the hardware snippet you want.

Examples:

- `DcsBios::EasyMode::Servo_SG90 ...`
- `DcsBios::EasyMode::Stepper ...`
- `DcsBios::EasyMode::Stepper_28BYJ48 ...`

### Where To Paste It

Paste the main object line near the top of the blank sketch, above `setup()`.

If the example also tells you to add extra tuning lines such as:

- `setMaxAngle(...)`
- `setMinAngle(...)`
- `setTrimDeg(...)`
- `setModulusEnabled(...)`

put those lines inside `setup()`, before `DcsBios::setup();`

### What You Usually Change First

A beginner will usually only need to change:

- telemetry source name
- Arduino pin numbers
- zero detection pin
- whether zero is at the start or in the middle
- maximum angle
- minimum angle
- trim

## The Simplest Copy-Paste Pattern

The blank sketch is designed around this simple idea:

1. Paste the Bort snippet above `setup()`
2. Paste any extra adjustment lines into `setup()`
3. Upload

Example:

```cpp
#define DCSBIOS_DEFAULT_SERIAL
#include "DcsBiosEasyMode.h"

DcsBios::EasyMode::Servo_SG90 altimeterNeedle(
    CommonData_ALT_MSL_FT_A,
    5
);

void setup() {
    altimeterNeedle.setTrimDeg(3);
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}
```

That is the basic pattern for most Easy Mode builds.

## What Each Example Is For

The example sketches in this library are not just random demos. Each one is meant to answer a common beginner question.

### 0_DefaultSerial

File:
`examples/0_DefaultSerial/0_DefaultSerial.ino`

What it is:

- A clean starting sketch for copy-paste use.
- The best first file for someone using Bort.

Use it when:

- You want to start from a blank project.
- You want to paste one Bort snippet into a known-good template.

Things the user will usually change:

- pasted object lines
- pasted setup lines

Photo placeholder:

- Add screenshot of the blank example open in Arduino IDE.

### Altimeter_EasyServo

File:
`examples/Altimeter_EasyServo/Altimeter_EasyServo.ino`

What it is:

- A generic servo example.
- A more detailed servo setup where all the main servo settings are visible in the first line.

Use it when:

- You are not using an SG90-style shortcut
- You want to see the full generic servo form

Usually changed by the user:

- servo pin
- minimum angle
- maximum angle
- direction
- trim

Important warning:

- A normal hobby servo is not a good real-world choice for a multi-turn altimeter needle.

Photo placeholder:

- Add photo of a generic hobby servo on a demonstration dial.

### Altimeter_EasyServo_SG90

File:
`examples/Altimeter_EasyServo_SG90/Altimeter_EasyServo_SG90.ino`

What it is:

- The shortest SG90-based servo example.
- A good first success example when using an SG90 or MG90 style servo.

Use it when:

- You want the shortest copy-paste servo setup
- You will change angles and trim later in `setup()`

Usually changed by the user:

- servo pin
- maximum angle
- trim

Photo placeholder:

- Add photo of an SG90 or MG90 mounted behind an instrument face.

### 1_Altimeter

File:
`examples/1_Altimeter/1_Altimeter.ino`

What it is:

- A bounded generic stepper example.
- A good model for gauges that need more travel than a servo can provide.

Use it when:

- You have a generic 4-wire stepper
- The gauge starts at one end of the scale and moves upward from there

Usually changed by the user:

- motor pins
- zero detection pin
- whether zero is at the start or in the middle
- maximum angle

Important idea:

- The hardware connection is in the object line.
- The real instrument travel is adjusted later with `setMaxAngle()`.

Photo placeholder:

- Add photo of a generic stepper and driver board connected to an altimeter dial.

### 91_Altimeter_EasyStepper

File:
`examples/91_Altimeter_EasyStepper/91_Altimeter_EasyStepper.ino`

What it is:

- The same altimeter idea as above, but for the common 28BYJ-48 and ULN2003 board.

Use it when:

- You have a 28BYJ-48 stepper
- You want a beginner-friendly stepper example with fewer setup details

Usually changed by the user:

- motor pins
- zero detection pin
- whether zero is at the start or in the middle
- maximum angle

Photo placeholder:

- Add photo of a 28BYJ-48 and ULN2003 board driving an altimeter dial.

### 4_Compass_Heading

File:
`examples/4_Compass_Heading/4_Compass_Heading.ino`

What it is:

- A continuous generic stepper example.
- A good starting point for repeating 360 degree instruments.

Use it when:

- The instrument goes around and around
- 360 degrees should behave the same as 0 degrees

Usually changed by the user:

- motor pins
- zero detection pin
- whether zero is at the start or in the middle
- whether modulus wrapping stays on

Important idea:

- This example is for repeating motion, not a fixed end-stop sweep.

Photo placeholder:

- Add photo of a compass or heading repeater driven by a generic stepper.

### 92_Compass_Heading

File:
`examples/92_Compass_Heading/92_Compass_Heading.ino`

What it is:

- The same heading idea as the generic compass example, but with a 28BYJ-48.

Use it when:

- You have a 28BYJ-48 stepper
- You want a 360 degree repeating instrument

Usually changed by the user:

- motor pins
- zero detection pin
- whether zero is at the start or in the middle
- whether modulus wrapping stays on

Photo placeholder:

- Add photo of a 28BYJ-48 turning a compass card or heading pointer.

### 2_Pitch_EasyServo

File:
`examples/2_Pitch_EasyServo/2_Pitch_EasyServo.ino`

What it is:

- A centered-zero SG90 servo example using pitch.
- A good first example for signals that move up and down around a center mark.

Use it when:

- The instrument zero is in the middle
- The signal can go both negative and positive from the center

Usually changed by the user:

- servo pin
- instrument sweep angles
- direction

Important idea:

- The final `true` value tells the library that the signal zero is in the middle of the range.

Photo placeholder:

- Add photo of a nose-up nose-down pitch gauge with a center mark.

### 3_Rate_Of_Climb_from_Altitude_EasyServo

File:
`examples/3_Rate_Of_Climb_from_Altitude_EasyServo/3_Rate_Of_Climb_from_Altitude_EasyServo.ino`

What it is:

- An advanced SG90 servo example that creates rate of climb from altitude changes.
- A natural next step after `2_Pitch_EasyServo`.

Use it when:

- Your aircraft does not expose a direct VVI / variometer output
- You still want to drive a rate-of-climb gauge

Usually changed by the user:

- servo pin
- the update interval
- the assumed climb/descent range
- instrument sweep angles

Important idea:

- This example uses `CommonData_ALT_MSL_FT` and calculates feet per minute in the sketch.

Photo placeholder:

- Add photo of a derived rate-of-climb gauge driven from altitude metadata.

## Suggested Learning Path

For a natural progression through the examples:

- Start with `1_Altimeter` to learn a simple one-way gauge.
- Move to `2_Pitch_EasyServo` to learn centered-zero gauges.
- Then try `3_Rate_Of_Climb_from_Altitude_EasyServo` to learn how to derive a gauge value from other telemetry.
- Continue with `4_Compass_Heading`, then the full blind-panel examples.

## Which Example Should I Start With?

If you have:

- an SG90 or MG90 servo: start with `Altimeter_EasyServo_SG90`
- another hobby servo: start with `Altimeter_EasyServo`
- a 28BYJ-48 stepper and a one-way gauge: start with `1_Altimeter`
- a generic 4-wire stepper and a one-way gauge: start with `91_Altimeter_EasyStepper`
- a repeating 360 degree gauge: start with one of the `Compass_Heading_*` examples
- a centered-zero gauge: start with `2_Pitch_EasyServo`
- a derived vertical-speed gauge: start with `3_Rate_Of_Climb_from_Altitude_EasyServo`
- no idea where to start and you just want to paste from Bort: start with `0_DefaultSerial`

## Common Beginner Mistakes

These are the most common first-time problems:

### The sketch uploads, but the gauge does not move

Check:

- DCS is running
- DCS-BIOS is installed and exporting data
- the correct telemetry item was chosen in Bort
- the board and COM port are correct in Arduino IDE
- wiring is correct

### The gauge moves the wrong way

Check:

- reverse setting
- motor wire order
- printed dial direction

### The gauge moves, but the sweep is too small or too large

Check:

- `setMinAngle(...)`
- `setMaxAngle(...)`
- gearing between the motor and the needle

### The zero point is wrong

Check:

- trim
- zero detection sensor position
- whether zero should be at the low end or in the middle

## Notes For Later PDF Release

Before turning this into a PDF for release, it would be useful to add:

- a screenshot of Arduino IDE with the blank sketch open
- a screenshot of Bort with a snippet selected
- one hardware photo for each example
- a simple servo wiring diagram
- a simple generic stepper wiring diagram
- a simple 28BYJ-48 plus ULN2003 wiring diagram
- a one-page troubleshooting appendix

## Drafting Note

This work was created as a guided collaboration between a human project lead and an AI acting as a virtual software engineer. The human role was not mainly to sit and type code line by line. Instead, the human role was closer to that of a team manager: setting the direction, explaining the goals in plain English, deciding what problems mattered most, reviewing the results, and steering the project toward something genuinely useful for cockpit builders. In practical terms, the human side managed the product vision while the AI handled much of the day-to-day software implementation. From the first discussion to the completed code changes, examples, Bort updates, and documentation draft, the work took less than a day.

The AI contribution went beyond simple code generation. It was used as a working software engineer that could take an English discussion, understand the intended outcome, and then turn that into real library changes, updated examples, improvements to Bort, and supporting documentation. That made it possible for the project to move from idea to implementation quickly, even though the human side did not generally write most of the code directly.

An important part of that work was research. The AI was able to study the surrounding context of DCS World, flight simulators, virtual cockpit building, instrument behavior, and the kinds of telemetry and hardware combinations that hobby builders actually use. That research helped the project avoid becoming a purely software-focused exercise. Instead, it supported decisions that made sense for real gauges, real panels, and real users trying to copy, paste, wire, and test their own cockpit parts.

That same collaboration also helped identify a broader design problem: DCS-BIOS and Bort were powerful tools, but they often assumed more software knowledge than many builders have or want to have. Many users are strong in aircraft systems, electronics, and physical fabrication, but may not be comfortable with programming language, software abstractions, or reading raw code. A major goal of this project was therefore to reduce that barrier and reshape the experience so it felt more welcoming to non-programmers.

With that goal in mind, the AI helped reshape both the code and the user experience. It contributed to the design and implementation of DCS-BIOS Easy Mode, created example sketches based on real DCS World telemetry, modified Bort so its generated snippets were easier to understand and copy into Arduino IDE, and helped produce beginner-friendly instructions for installing and using the whole toolchain. The result is not only a set of software changes, but also a more accessible workflow for people who want to build virtual cockpit hardware without needing to think like professional software developers.



