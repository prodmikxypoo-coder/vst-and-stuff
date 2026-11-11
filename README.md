# Panda Rage

A one-knob distortion VST3 with a panda UI. Turn **Rage** up and the panda goes from playful and
colorful to a horned, red-eyed panda god while the screen (and the audio) fills with noise.

## Build (Windows)

1. Install **Visual Studio 2022 Community** with the "Desktop development with C++" workload.
2. Install **CMake** and **Git** (both need to be on your PATH).
3. In this folder, open a terminal and run:

   ```
   cmake -B build
   cmake --build build --config Release
   ```

   The first run downloads JUCE (a few minutes).
4. The VST3 is copied to `C:\Program Files\Common Files\VST3` automatically. If that fails with a
   permissions error, run the terminal as Administrator, or copy
   `build\PandaRage_artefacts\Release\VST3\Panda Rage.vst3` there yourself.

## Load it in FL Studio

1. Options > Manage plugins > **Find plugins** (make sure `C:\Program Files\Common Files\VST3` is in the scan list).
2. Add "Panda Rage" to a mixer insert as an effect.

## Testing safely

- Put a limiter on the master and keep your volume low the first time.
- The plugin has a built-in hard clip at about -0.2 dBFS, but still be careful with headphones.
- Test in an empty project first. A plugin bug can crash the host.

## Standalone test

The build also makes a standalone app (`build\PandaRage_artefacts\Release\Standalone`) so you can
check the UI without opening FL.

## Ideas for later

- Mix (dry/wet) and output knobs
- Oversampling to cut aliasing at high drive
- Tone/filter after the distortion
- Panda reacts to input level, not just the knob
