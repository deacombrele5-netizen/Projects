# Terrain Tools — Minecraft Forge 1.12.2

A standalone mining utility mod for Forge 1.12.2.

## Features

- Adds the **Terrain Pickaxe** to the Tools creative tab.
- Press **G** to toggle 3×3 area mining. The key can be rebound in Controls.
- The 3×3 plane follows the direction the player is facing.
- Only the special pickaxe is affected; normal tools behave normally.
- Skips unloaded positions, tile entities (for example chests and machines), unbreakable blocks, and blocks with hardness above 4.
- Extra blocks are harvested through the normal server interaction manager, preserving vanilla/Forge block-break checks, drops, and tool durability.
- Russian and English language strings.

## Requirements

- Minecraft 1.12.2
- Minecraft Forge 14.23.5.2860
- Java 8 (for building)

## Build

From this directory run:

```sh
gradle setupDecompWorkspace --no-daemon
gradle build --no-daemon
```

The compiled JAR is created in `build/libs/`.

## Install

Copy the resulting JAR into the Minecraft instance's `mods` folder. Install Forge 1.12.2 first.

This mod is a mining utility. It does not add explosive or weapon behavior.
