# ZHImpact — Forge 1.16.5

A configurable damage laboratory for singleplayer worlds and Forge servers you own or administer.

## Features
- Minecraft Forge 1.16.5 / Java 8.
- Classic in-game menu opened with **U** (the physical key works as U/Г in either keyboard layout).
- Enable/disable switch.
- Positive multiplier, zero-damage mode, and negative reflection mode.
- Server-side common config: `config/zhimpact-common.toml`.
- Russian and English keybind names.

## Multiplayer behavior
Damage is changed by the authoritative logical server. Installing only the client mod does not force Envy World, Dead Rate, Really World, or another remote server to accept modified damage. The server owner must install and configure this mod. No anticheat bypasses, packet spoofing, or stealth features are included.

## Build
Use Java 8 and Gradle 6.8.3:
```sh
gradle build
```
The jar is created in `build/libs/`.

## Configuration
Edit `config/zhimpact-common.toml` on the server:
- `enabled = true`
- `multiplier = 1.0`

The multiplier is finite and bounded to ±1,000,000,000. Literal infinity is not supported because it can break health and damage calculations.
