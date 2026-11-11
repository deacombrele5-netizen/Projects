# Living World: Civilization

Minecraft Java Edition 1.16.5 — Forge 36.2.41.

## Prototype commands

- `/lw start` — begin a staged city layout at the command position (requires permission level 2).
- `/lw status` — show the current phase, block progress, nearby adult villager count and the prototype worker inventory total.
- `/lw stop` — stop the active layout (requires permission level 2).

## Current implemented prototype

- Builds a visible staged layout: two intersecting roads, sidewalks/lamps, four starter homes, a town square, civic building, shops, a small pool and a high-rise landmark.
- Places a limited number of blocks per world tick so the build is visibly progressive rather than appearing instantly.
- Nearby adult vanilla villagers are recruited as prototype workers. They navigate toward nearby logs/stone in the outer work area, equip an iron axe or pickaxe, swing it and break eligible resource blocks so the normal block drops are produced.
- Right-click an adult villager while holding an item to transfer a stack into that villager's prototype 16-slot worker inventory.
- `/lw status` reports worker count and the total number of stored items.

## Important limitations — not a finished civilization simulation

This is an early technical milestone, not the full feature set requested. Worker inventories currently exist only in memory and are not saved to the world; transferred items are stored but are not yet consumed by construction. Villager mining is a simple nearby-resource routine, not a complete profession/job system. The city layout is a fixed starter template started manually with `/lw start`; it does not yet discover and independently evolve every generated village. There is no persistent economy, autonomous city planning, road traffic, raid/defence simulation, voice chat, or full city GUI yet.

Use a **copy of your world** for testing: the prototype places blocks and can remove nearby logs/stone while workers gather resources. Stop it with `/lw stop` if you want to halt construction.

## Build

GitHub Actions builds the Forge mod and publishes a `LivingWorld-Forge-1.16.5` artifact. Compilation success does not replace an in-game test; confirm behavior in a disposable single-player world before using it in a main save.
