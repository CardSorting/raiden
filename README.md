# Sky Circuit

Sky Circuit is a complete local C++17/raylib vertical mobile-first arcade shooter. It keeps the theatre of 1990s cabinet shmups, but the primary play assumptions are handheld readability, thumb-driven steering, short sessions, compressed pacing, generous telegraphs, and stable performance.

## Handoff Quick Start

### 1. Build

Build from the project root with CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The first configure may fetch Raylib 5.5 if it is not already installed.

### 2. Launch

- **macOS / Linux**:
  ```bash
  ./build/sky_circuit
  ```
- **Windows**:
  ```powershell
  .\\build\\Release\\sky_circuit.exe
  ```

### 3. Start A Run

When the game starts up, select **Start Run** to enter **Route Vantage**:
- **Touch / Mouse Navigation**: Tap or hover over **Start Run** and select it.
- **Keyboard Navigation**: Press **Up/Down** or **W/S** to move the cursor pointer `>` and press **Enter** or **Space** to confirm.
- **Gamepad Navigation**: Press **D-Pad Up/Down** and confirm with the **Down Face Button (A / Cross)**.

### 4. Play

Once the mission starts, a helper banner appears near the bottom of the playfield:
- **Touch / Mouse Drag**: Drag below the ship to steer. The ship floats above the thumb/finger so it stays visible.
- **Auto-Fire**: Weapons fire continuously during play.
- **Bomb**: Two-finger tap on touch screens, right-click with mouse, or press the mapped bomb button.
- **Keyboard / Gamepad Compatibility**: Keyboard and controller inputs remain available for desktop play.
*Note: You can switch control presets, adjust volume sliders, or toggle fullscreen modes inside the **Tuning** menu at any time.*

## What This Is

**Route Vantage** is the current authored arcade route. It is a short mobile-first run built around readable phase identity:

- **Opening / Sweep / Interceptors**: establish movement language and single-purpose threats.
- **SAFE LANE**: a recovery phase that clears pressure and lets the player re-center.
- **BONUS PARADE**: a low-stress score and rhythm section with synchronized target motion.
- **Threat Horizon / Fortress Corridor**: short readable pressure spikes, not bullet sludge.
- **Tactical Silence / Boss Approach**: ritual buildup before VANTAGE-9 arrives.
- **Boss / Route Clear**: theatrical duel, readable weak-point feedback, medal burst, and score payoff.

Judge it as a compact mobile arcade route, not as a campaign, RPG, or content platform.

## Features

- 480x640 vertical mobile playfield.
- Mobile-first title shell with **Start Run**, **Route Brief**, **Tuning**, **Scores**, and **Exit**.
- **Tuning / Settings Screen**: Sliders for SFX and Music volume, Screen Shake toggle, Screen Mode (Windowed/Fullscreen), and Hitbox display toggle.
- **Persistent High Scores Leaderboard**: Hall of Fame saving to local `highscores.txt` with Name Entry UI for player initials.
- **Procedural Audio & Chiptune BGM**: Fully synthesized retro chiptune loop, sound effects, explosions, and UI tones generated entirely in code.
- **Attract Mode (Gameplay Demo)**: Automatically starts an auto-play demonstration controlled by a dodging/shooting AI when left idle for 24 seconds.
- **Gamepad / Controller Support**: Full controller integration for gameplay and menus.
- In-game help screen with objective, controls, pickups, and survival tips.
- Pause, stage-clear, and game-over states.
- Pause-on-focus-loss behavior for app/lifecycle safety.
- Touch/mouse drag steering with thumb offset, plus keyboard/gamepad compatibility.
- Auto-fire weapons tuned for movement-first handheld play.
- Deliberately small visible red hitbox for fair bullet reading.
- Weapons:
  - **Vulcan**: fast straight shot with upgraded spread.
  - **Plasma**: slower wide coverage.
  - **Missile**: secondary homing projectiles.
- Bomb button clears enemy bullets, damages enemies, grants brief invulnerability, and shakes the screen.
- Powerups: weapon change, weapon upgrade, bomb, and score medal.
- Authored mobile-first stage progression with named wave blocks, clear transition callouts, an earlier reward-heavy bonus formation, recovery windows, short pressure spikes, and an expanded boss runway.
- Miniboss with slower entrance, clearer weak point, wider safe lanes, and reduced projectile overlap.
- Stage clear restarts Route Vantage as a tighter score loop.
- Particle explosions, HUD, scrolling layered background, and developer-facing debug controls.

## Controls

| Action | Keyboard | Gamepad | Mouse |
| --- | --- | --- | --- |
| Menu selection | Up/Down or W/S | D-Pad Up/Down | Hover cursor over option |
| Confirm selection | Enter or Space | Face Button Down (A / Cross) | Left-click option |
| Adjust volume sliders | Left/Right or A/D | D-Pad Left/Right | Drag slider handle |
| Back from submenus | Enter, Escape, or Backspace | Face Button Down / Right (A/B) | Left-click BACK button |
| Move | Arrow keys or WASD | Left Analog Stick or D-Pad | Drag below ship |
| Focus / Slow movement | Left Shift | Shoulder Triggers (L1 / R1) | - |
| Shoot | Auto-fire, Z, or Space | Auto-fire / Face Button Down or Left | Auto-fire |
| Bomb | X | Face Button Right (B / Circle) | Two-finger tap / right click |
| Pause / Resume | P | Menu/Start Button | Navigate pause menu choices |
| Toggle hitboxes | F1 (in gameplay) | Configurable in Tuning | - |
| Hold diagnostics | F3 while hitboxes are enabled | - | - |

## Tester Checklist

Use this checklist when evaluating the current release candidate:

- **Touch movement feel**: Drag steering should feel connected, smooth, and not slippery.
- **Auto-fire readability**: Continuous fire should support movement-first play without visual overload.
- **Two-finger bomb reliability**: Two-finger tap should trigger a bomb without yanking the ship toward the first touch point.
- **BONUS PARADE clarity**: Bonus targets should read as joyful, low-stress, rhythmic, and score-heavy.
- **SAFE LANE clarity**: Recovery phases should visibly reduce pressure and communicate safety quickly.
- **Boss approach readability**: VANTAGE-9 should feel introduced, telegraphed, and readable before combat begins.
- **Retry speed**: Continue, Game Over, and Run It Back should return to action quickly without credit friction.
- **Pause-on-focus-loss behavior**: Losing app/window focus during play should pause instead of letting the run continue.
- **Performance smoothness**: Watch for frame drops during explosions, boss collapse, and bonus chains.
- **Small-screen clutter**: Check whether bullets, medals, score text, and ritual overlays remain legible on a phone-sized viewport.

## What To Judge

Useful feedback should focus on:

- Whether the first 60 seconds become interesting quickly enough.
- Whether the player always understands safe, danger, bonus, silence, and boss states.
- Whether thumb steering keeps the ship visible and responsive.
- Whether BONUS PARADE feels worth replaying.
- Whether boss danger comes from reaction and timing, not visual confusion.
- Whether the retry loop creates “one more run” momentum.
- Whether any UI element still feels desktop-first, tiny, or debug-shaped.

Avoid judging it by content volume, unlock depth, campaign length, or progression hooks; those are intentionally out of scope.

## Debug Controls

Debug tools are developer-facing and should not be part of normal tester evaluation:

- **F1** toggles hitbox/debug rendering and persists the hitbox setting.
- **F3 held while F1 debug is enabled** shows the stage diagnostics overlay.
- The Tuning screen can toggle hitbox display for collision inspection.
- The default presentation has hitboxes off, CRT shader off, clean pixel mode on, and bezel off.

## Build

This project uses CMake. It first tries `find_package(raylib)`. If raylib is not installed, CMake fetches raylib 5.5 from GitHub as the only third-party dependency.

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/sky_circuit
```

### Windows

From a Developer PowerShell or terminal with CMake and a C++ compiler installed:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\\build\\Release\\sky_circuit.exe
```

If using MinGW or Ninja single-config generators, the executable may be at `build/sky_circuit.exe`.

## Project Structure

```text
.
├── CMakeLists.txt          # CMake build and raylib dependency resolution
├── main.cpp                # Entry point
├── Game.h/.cpp             # State machine, orchestration, collisions, rendering, sounds
├── Player.h/.cpp           # Movement, weapons, bombs, player rendering
├── Enemy.h/.cpp            # Popcorn, turret, and miniboss behavior
├── Bullet.h/.cpp           # Projectile movement, collision radius, homing missiles
├── Powerup.h/.cpp          # Collectible item behavior
├── StageDirector.h/.cpp    # Authored arcade stage, section, and wave block choreography
└── Effects.h/.cpp          # Particles and screen shake
```

## Mobile-First Migration Touch Points

The mobile-first migration work is concentrated in these files:

- `Game.cpp` / `Game.h`: shell flow, state machine, HUD, transitions, route rituals, pause/resume behavior, cleanup caps, boss/clear/session flow.
- `Player.cpp` / `Player.h`: touch drag steering, auto-fire, two-finger bomb handling, movement feel.
- `StageDirector.cpp` / `StageDirector.h`: authored Route Vantage pacing, phase timing, formation choreography, bonus/recovery/boss runway structure.
- `Enemy.cpp` / `Enemy.h`: boss readability, attack cadence, telegraphs, enemy behavior tuning.
- `Effects.cpp` / `Effects.h`: particle/text/effect caps and readable destruction feedback.
- `settings.cfg`: mobile-first presentation defaults.
- `README.md`: handoff, route identity, tester checklist, and preservation guardrails.

## Design Notes

- The game uses simple vectors and circle collisions for clarity and fast iteration.
- Procedural vector art and tones are intentional: the game has no copyrighted sprites, logos, music, or level layouts.
- The gameplay loop is compact and mobile-first: quick orientation, early spectacle, short pressure bursts, a low-stress score parade, readable boss approach, boss combat, stage clear, then repeat as a tighter route loop.
- Stage pacing is directed through explicit wave blocks: opening orientation, sweep language, formation reassembly, aimed interceptors, false recovery pinch, breathing lane, debris drift, bonus formation, threat horizon, fortress corridor, fortress collapse, tactical silence, boss arrival, and boss combat.
- The red center dot is the actual player hitbox; the ship body may overlap bullets without counting as a hit.

## Identity Preservation Guardrails

Sky Circuit should stay a focused mobile arcade route, not a progression platform. Future changes should preserve:

- **Route identity**: Keep **Route Vantage**, **Gateline-01**, SOLACE, VANTAGE-9, the gate motif, and the bonus parade as recurring anchors. Add lore sparingly and only when it reinforces the ritual.
- **Mobile readability**: Prefer broad formation motion, strong silhouettes, clear safe lanes, short projectile bursts, and explicit phase color language. Do not increase density to create difficulty.
- **Replay flow**: Preserve free-play start, immediate retry, short menu depth, and fast emotional payoff. Avoid mandatory upgrades, reward-claim screens, nested menus, or unlock friction.
- **Bonus protection**: **BONUS PARADE** should remain low-stress, rhythmic, score-heavy, and celebratory. Do not add hidden threats or punishment mechanics to it.
- **Boss ceremony**: Bosses should remain introduced, readable, and theatrical. Telegraph posture before attacks and protect visible safe lanes.
- **Expansion philosophy**: If new content is added, prefer formation choreography, transition rituals, emotional palette shifts, and timing variation over new currencies, inventories, RPG stats, or enemy bloat.

## Verification

The current source has been verified with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Honest Limitations

- There is no external art or music pipeline; presentation is intentionally procedural.
- Desktop is supported but secondary; the primary target is a phone-sized vertical playfield and touch-style control feel.
- Mobile packaging/deployment is not included yet; this is currently a local C++/raylib build.
- The stage is a compact arcade route with named act progression rather than a multi-level campaign.
- BONUS PARADE and the boss are intentionally restrained to preserve readability and replay rhythm.
- Debug tools are developer-facing and should not be treated as part of the shipped player experience.
- High scores are saved persistently to `highscores.txt` in the working directory.
- There are no currencies, unlock trees, inventory systems, daily tasks, or live-service loops by design.
