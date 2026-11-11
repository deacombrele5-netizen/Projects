# ZH Builder — Forge 1.16.5

ZH Builder is a server-authoritative building assistant for Minecraft Forge 1.16.5.

## Current build (0.1.0)

- Server commands for two-point region selection.
- Copy and paste block structures, including block-entity NBT where supported.
- Save and load selections as portable `.zhb` schematic files in the server's `zhbuilder/schematics` folder.
- Optional OpenAI-compatible AI generation endpoint. The server operator supplies the endpoint, model and API key in `config/zhbuilder-common.toml`.
- Client prompt screen opened with **U**; it sends normal ZH Builder commands to the server.
- All world edits are performed by the server and require operator permission (permission level 2). This is intentional: a client-only mod cannot force edits on an unrelated server.

## Install

1. Install Minecraft Java 1.16.5 and Forge 36.2.39.
2. Put `ZHBuilder-0.1.0.jar` in the server's `mods` folder.
3. For the prompt screen, also put the same jar in the clients' `mods` folder.
4. Start the server. Operators can use `/zhb help`.

## Commands

- `/zhb pos1` — set selection corner 1 at your feet.
- `/zhb pos2` — set selection corner 2 at your feet.
- `/zhb copy` — copy the selected region into your in-memory clipboard.
- `/zhb paste` — paste the clipboard at your current position.
- `/zhb save <name>` — save the clipboard to `zhbuilder/schematics/<name>.zhb`.
- `/zhb load <name>` — load a saved schematic into your clipboard.
- `/zhb generate <description>` — ask the configured AI endpoint for a block plan and place it at your position.
- `/zhb help` — list commands.

The initial release copies block states and available block-entity data. It does not clone entities, inventories of players, command history, or an entire remote server world. Whole-world backups must be made by the server owner from the server's normal world-save directory.

## AI setup

Edit the server's `config/zhbuilder-common.toml`:

- `aiEnabled = true`
- `aiEndpoint` — OpenAI-compatible chat-completions URL.
- `aiModel` — model identifier supported by that endpoint.
- `aiApiKey` — private API key; never commit it to GitHub.

The model is instructed to return a bounded JSON block plan. The server validates block IDs and caps the generated plan before placing anything. AI generation requires network access and a valid provider key; copy/paste works without AI.

## Safety and server compatibility

The mod does not bypass server permissions or anti-cheat. The server must have the mod installed and the operator must have permission level 2. On a server without ZH Builder, the mod cannot write blocks or export that server's map.

## Build

Requires Java 8 and Gradle 6.8.3:

```bash
gradle build --no-daemon
```

The distributable jar is in `build/libs/`.
