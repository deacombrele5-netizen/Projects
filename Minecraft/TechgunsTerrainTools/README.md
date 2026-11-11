# Terrain Tools

A lightweight mining utility for Minecraft Forge 1.12.2.

## Features

- Adds the Terrain Pickaxe to the Tools creative tab.
- Press **G** to toggle 3×3 area mining. The key can be changed in **Options → Controls**.
- The mining area follows the player's horizontal facing direction.
- The feature only applies while using the Terrain Pickaxe.
- Skips unloaded positions, tile entities, unbreakable blocks, and blocks with hardness above 4.
- Uses the server's normal block-harvesting process for extra blocks, including Forge break events, permissions, drops, and tool durability.
- Includes English and Russian language files.

## Requirements

- Minecraft 1.12.2
- Minecraft Forge 14.23.5.2860
- Java 8 for compiling

## Build

Run this command from the project directory:

```sh
gradle build --no-daemon
```

The JAR is generated in `build/libs/`.

## Install

1. Install Minecraft Forge 1.12.2.
2. Copy the Terrain Tools JAR into the instance's `mods` folder.
3. Launch the game.

## Compatibility

Terrain Tools is a standalone mining utility. It does not require Techguns and does not add weapon or explosive functionality.
