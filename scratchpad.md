# Hardening Audit Pass Plan

## Objective
Perform a second, deeper audit and hardening pass on `Sky Circuit` to make the game feel less like a raw developer build and more like a complete local arcade cabinet experience: clearer onboarding, familiar menu navigation for non-technical users, robust state transitions, cleaner terminology, stricter compiler checks, and verified builds.

## Findings From Audit
- Some user-facing text undersold the procedural art/audio direction. The implementation already uses real procedural vector art and generated tones, so user-facing copy should present those as the game's production style.
- Navigation was too developer-centric: the title screen only offered an instant start prompt with controls compressed into one line. Non-technical users benefit from familiar menu patterns: highlighted choices, arrow-key navigation, a dedicated How To Play screen, and explicit back/confirm prompts.
- Exit flow was implicit through the window close button. A menu Exit option backed by a `shouldExit_` flag makes the path clear.
- Build quality should include common warning flags on Clang/GCC/MSVC, not just a successful default compile.

## JoyZoning Architectural Mapping

### DOMAIN
- Gameplay rules in `Player`, `Enemy`, `Bullet`, `Powerup`, `StageDirector`, and `Effects` remain deterministic and simple.
- DOMAIN probe: no new disk/network I/O is introduced; the added navigation does not alter pure gameplay rules.

### CORE
- `Game` state machine expands from Title/Playing/Paused/StageClear/GameOver to include HowTo and menu selection.
- CORE probe: `Game.cpp` coordinates state transitions and input; it must avoid burying game mechanics in menu code.

### INFRASTRUCTURE
- Existing raylib window/audio/build integrations remain the only infrastructure layer.
- INFRASTRUCTURE probe: CMake remains simple; no new external assets, engines, accounts, or services. Warning flags improve build hygiene without changing runtime behavior.

### UI
- Title menu, How To Play screen, and pause instructions are user-facing UI improvements.
- UI probe: non-technical users should be able to discover start/help/exit and controls from inside the game without reading README.

## Implementation Steps
1. Add menu selection and How To Play state to `Game`.
2. Replace underconfident wording in user-facing copy with procedural-asset terminology.
3. Update README with clearer non-technical quick start, controls, menu navigation, and honest limitations.
4. Add common compiler warnings for Clang/GCC/MSVC.
5. Re-run residue search and CMake build.

## Sovereign Triad Audit

### THE ARCHITECT (Boundary Probe)
Most vulnerable boundary is `Game.cpp`, where UI navigation and core state machine live together. Keep the change focused on state transitions and rendering; do not move weapon/enemy/collision rules into the menu implementation. File paths: `Game.h`, `Game.cpp`, `README.md`, `CMakeLists.txt`.

### THE CRITIC (Assumption Probe)
Assumption: users understand arrow-key menu navigation and Enter/Escape patterns. Harden by rendering explicit prompts on every non-gameplay screen and keeping Start Mission selected by default for immediate play.

### THE SRE (Atomic Probe)
If a patch fails mid-pass, recovery is to restore the small edited set (`Game.h`, `Game.cpp`, `README.md`, `CMakeLists.txt`, `scratchpad.md`) and rebuild. No persistent saves, migrations, or external assets are introduced, so rollback is source-only and build artifacts remain disposable under `build/`.

Mantra: *Double down on this concept, audit and revise in its entirety.*
