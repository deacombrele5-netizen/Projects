# Living World: Civilization

Forge 1.16.5 / 36.2.41. Early playable prototype.

## Commands
- `/lw start` — start a staged city project around the command location (OP level 2).
- `/lw status` — show construction stage and progress.
- `/lw stop` — stop the current project (OP level 2).

The prototype lays out a road grid, several starter buildings, a civic centre, shops, a pool and a landmark tower in visible batches. Nearby adult villagers are equipped with pickaxes/axes and move toward the work area as a first construction-worker visual.

## Current limitations
This is a foundation, not the complete simulation. It does not yet implement autonomous resource extraction/inventory economies, villager-specific construction AI, raids/combat extensions, persistent city save data, or voice interaction. The next milestone is proper job AI, tool use tied to block harvesting, item hand-off, pathfinding work sites, city persistence, and safe configurable build boundaries. Test in a copied world first: the command places blocks in the loaded world.
