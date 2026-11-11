# Terrain Tools — Minecraft Forge 1.12.2

Standalone add-on: a special **Terrain Pickaxe** with optional 3×3 area mining.

- Press **G** (rebindable in Controls) to toggle the mode.
- Only affects the special pickaxe; ordinary tools are unchanged.
- Mines a 3×3 plane aligned to the player's horizontal facing.
- Skips unbreakable blocks, blocks above hardness 4.0, unloaded positions, and tile entities such as chests/machines.
- Additional blocks consume pickaxe durability.
- Russian and English localization.

## Build
Requires JDK 8 and Gradle 4.10.x. Run `gradle setupDecompWorkspace` then `gradle build`. JAR: `build/libs/`.

Install the JAR in the Minecraft 1.12.2 Forge `mods` folder. The item appears in the Tools creative tab.

This is a mining utility and does not add explosive or weapon behavior.
