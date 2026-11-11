package net.zadrot.zhbuilder.command;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.brigadier.arguments.StringArgumentType;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.command.Commands;
import net.minecraft.command.CommandSource;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.nbt.CompressedStreamTools;
import net.minecraft.nbt.CompoundNBT;
import net.minecraft.util.ResourceLocation;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.text.StringTextComponent;
import net.minecraft.world.server.ServerWorld;
import net.minecraftforge.event.RegisterCommandsEvent;
import net.minecraftforge.registries.ForgeRegistries;
import net.zadrot.zhbuilder.ZHBuilder;
import net.zadrot.zhbuilder.config.BuilderConfig;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;

public final class BuilderCommands {
    private static final Map<UUID, BlockPos> POS1 = new HashMap<>();
    private static final Map<UUID, BlockPos> POS2 = new HashMap<>();
    private static final Map<UUID, ClipboardStore> CLIPBOARDS = new HashMap<>();

    private BuilderCommands() {}

    public static void register(RegisterCommandsEvent event) {
        event.getDispatcher().register(Commands.literal("zhb")
            .requires(source -> source.hasPermission(2))
            .then(Commands.literal("help").executes(ctx -> {
                ctx.getSource().sendSuccess(new StringTextComponent("/zhb pos1, pos2, copy, paste, save <name>, load <name>, generate <description>"), false);
                return 1;
            }))
            .then(Commands.literal("pos1").executes(ctx -> setPosition(ctx.getSource(), true)))
            .then(Commands.literal("pos2").executes(ctx -> setPosition(ctx.getSource(), false)))
            .then(Commands.literal("copy").executes(ctx -> copy(ctx.getSource())))
            .then(Commands.literal("paste").executes(ctx -> paste(ctx.getSource())))
            .then(Commands.literal("save").then(Commands.argument("name", StringArgumentType.word())
                .executes(ctx -> save(ctx.getSource(), StringArgumentType.getString(ctx, "name")))))
            .then(Commands.literal("load").then(Commands.argument("name", StringArgumentType.word())
                .executes(ctx -> load(ctx.getSource(), StringArgumentType.getString(ctx, "name")))))
            .then(Commands.literal("generate").then(Commands.argument("prompt", StringArgumentType.greedyString())
                .executes(ctx -> generate(ctx.getSource(), StringArgumentType.getString(ctx, "prompt")))))
        );
    }

    private static int setPosition(CommandSource source, boolean first) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        (first ? POS1 : POS2).put(player.getUUID(), player.blockPosition());
        source.sendSuccess(new StringTextComponent((first ? "Position 1" : "Position 2") + " set to " + player.blockPosition()), false);
        return 1;
    }

    private static int copy(CommandSource source) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        BlockPos a = POS1.get(player.getUUID()), b = POS2.get(player.getUUID());
        if (a == null || b == null) { source.sendFailure(new StringTextComponent("Set both corners first: /zhb pos1 and /zhb pos2")); return 0; }
        BlockPos min = new BlockPos(Math.min(a.getX(), b.getX()), Math.min(a.getY(), b.getY()), Math.min(a.getZ(), b.getZ()));
        BlockPos max = new BlockPos(Math.max(a.getX(), b.getX()), Math.max(a.getY(), b.getY()), Math.max(a.getZ(), b.getZ()));
        try {
            ClipboardStore clipboard = ClipboardStore.capture(player.getLevel(), min, max, BuilderConfig.MAX_COPY_BLOCKS.get());
            CLIPBOARDS.put(player.getUUID(), clipboard);
            source.sendSuccess(new StringTextComponent("Copied " + clipboard.entries.size() + " blocks (" + clipboard.sizeX + "x" + clipboard.sizeY + "x" + clipboard.sizeZ + ")."), false);
            return 1;
        } catch (IllegalArgumentException ex) {
            source.sendFailure(new StringTextComponent(ex.getMessage()));
            return 0;
        }
    }

    private static int paste(CommandSource source) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        ClipboardStore clipboard = CLIPBOARDS.get(player.getUUID());
        if (clipboard == null) { source.sendFailure(new StringTextComponent("Clipboard is empty. Use /zhb copy or /zhb load.")); return 0; }
        clipboard.paste(player.getLevel(), player.blockPosition());
        source.sendSuccess(new StringTextComponent("Pasted " + clipboard.entries.size() + " blocks."), false);
        return 1;
    }

    private static File schematicFile(String name) {
        if (!name.matches("[A-Za-z0-9_-]{1,48}")) throw new IllegalArgumentException("Use only letters, numbers, _ and - (max 48 characters).");
        File directory = new File("zhbuilder/schematics");
        if (!directory.exists() && !directory.mkdirs()) throw new IllegalStateException("Could not create schematic directory.");
        return new File(directory, name + ".zhb");
    }

    private static int save(CommandSource source, String name) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        ClipboardStore clipboard = CLIPBOARDS.get(player.getUUID());
        if (clipboard == null) { source.sendFailure(new StringTextComponent("Clipboard is empty.")); return 0; }
        try {
            CompressedStreamTools.writeCompressed(clipboard.toNbt(), schematicFile(name));
            source.sendSuccess(new StringTextComponent("Saved schematic: " + name + ".zhb"), false);
            return 1;
        } catch (Exception ex) {
            source.sendFailure(new StringTextComponent("Save failed: " + ex.getMessage()));
            return 0;
        }
    }

    private static int load(CommandSource source, String name) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        try {
            File file = schematicFile(name);
            if (!file.isFile()) { source.sendFailure(new StringTextComponent("Schematic not found: " + name)); return 0; }
            CompoundNBT nbt = CompressedStreamTools.readCompressed(file);
            ClipboardStore clipboard = ClipboardStore.fromNbt(nbt);
            if (clipboard.entries.size() > BuilderConfig.MAX_COPY_BLOCKS.get()) { source.sendFailure(new StringTextComponent("Schematic exceeds configured block limit.")); return 0; }
            CLIPBOARDS.put(player.getUUID(), clipboard);
            source.sendSuccess(new StringTextComponent("Loaded schematic: " + name + ".zhb"), false);
            return 1;
        } catch (Exception ex) {
            source.sendFailure(new StringTextComponent("Load failed: " + ex.getMessage()));
            return 0;
        }
    }

    private static int generate(CommandSource source, String prompt) throws com.mojang.brigadier.exceptions.CommandSyntaxException {
        ServerPlayerEntity player = source.getPlayerOrException();
        if (!BuilderConfig.AI_ENABLED.get() || BuilderConfig.AI_API_KEY.get().trim().isEmpty()) {
            source.sendFailure(new StringTextComponent("AI is not configured. Set aiEnabled=true and aiApiKey in config/zhbuilder-common.toml."));
            return 0;
        }
        if (prompt.length() < 4 || prompt.length() > 500) {
            source.sendFailure(new StringTextComponent("Prompt must contain 4-500 characters."));
            return 0;
        }
        UUID playerId = player.getUUID();
        BlockPos origin = player.blockPosition();
        ServerWorld world = player.getLevel();
        source.sendSuccess(new StringTextComponent("ZH Builder: asking the configured AI model for a build plan..."), false);
        CompletableFuture.runAsync(() -> {
            try {
                JsonObject plan = requestPlan(prompt);
                world.getServer().execute(() -> {
                    try {
                        JsonArray blocks = plan.getAsJsonArray("blocks");
                        if (blocks == null || blocks.size() == 0 || blocks.size() > BuilderConfig.MAX_AI_BLOCKS.get()) {
                            source.sendFailure(new StringTextComponent("AI plan is empty or exceeds the block limit."));
                            return;
                        }
                        int placed = 0;
                        for (int i = 0; i < blocks.size(); i++) {
                            JsonObject blockJson = blocks.get(i).getAsJsonObject();
                            int x = blockJson.get("x").getAsInt();
                            int y = blockJson.get("y").getAsInt();
                            int z = blockJson.get("z").getAsInt();
                            String id = blockJson.get("block").getAsString();
                            ResourceLocation key = new ResourceLocation(id);
                            Block block = ForgeRegistries.BLOCKS.getValue(key);
                            if (block == null || block == net.minecraft.block.Blocks.AIR) continue;
                            if (Math.abs(x) > 64 || Math.abs(y) > 64 || Math.abs(z) > 64) continue;
                            BlockPos target = origin.offset(x, y, z);
                            if (world.isOutsideBuildHeight(target) || !world.getWorldBorder().isWithinBounds(target)) continue;
                            BlockState state = block.defaultBlockState();
                            if (world.setBlock(target, state, 3)) placed++;
                        }
                        source.sendSuccess(new StringTextComponent("AI build placed: " + placed + " blocks."), false);
                    } catch (Exception ex) {
                        source.sendFailure(new StringTextComponent("Could not apply AI plan: " + ex.getMessage()));
                    }
                });
            } catch (Exception ex) {
                world.getServer().execute(() -> source.sendFailure(new StringTextComponent("AI request failed: " + ex.getMessage())));
                ZHBuilder.LOGGER.error("ZH Builder AI request failed", ex);
            }
        });
        return 1;
    }

    private static JsonObject requestPlan(String prompt) throws Exception {
        HttpURLConnection connection = (HttpURLConnection) new URL(BuilderConfig.AI_ENDPOINT.get()).openConnection();
        connection.setRequestMethod("POST");
        connection.setConnectTimeout(15000);
        connection.setReadTimeout(90000);
        connection.setDoOutput(true);
        connection.setRequestProperty("Content-Type", "application/json; charset=UTF-8");
        connection.setRequestProperty("Authorization", "Bearer " + BuilderConfig.AI_API_KEY.get());

        JsonObject body = new JsonObject();
        body.addProperty("model", BuilderConfig.AI_MODEL.get());
        body.addProperty("temperature", 0.2);
        JsonArray messages = new JsonArray();
        JsonObject system = new JsonObject();
        system.addProperty("role", "system");
        system.addProperty("content", "You are a Minecraft 1.16.5 structure planner. Return ONLY JSON: {\"blocks\":[{\"x\":0,\"y\":0,\"z\":0,\"block\":\"minecraft:stone\"}]}. Coordinates must be relative to the player's feet. Use only valid Minecraft 1.16.5 block IDs. Maximum 8192 entries. Keep all coordinates between -64 and 64. Do not include markdown.");
        JsonObject user = new JsonObject();
        user.addProperty("role", "user");
        user.addProperty("content", prompt);
        messages.add(system); messages.add(user);
        body.add("messages", messages);

        byte[] payload = body.toString().getBytes(StandardCharsets.UTF_8);
        try (OutputStream output = connection.getOutputStream()) { output.write(payload); }
        int status = connection.getResponseCode();
        InputStream stream = status >= 200 && status < 300 ? connection.getInputStream() : connection.getErrorStream();
        StringBuilder response = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(stream, StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) response.append(line);
        }
        connection.disconnect();
        if (status < 200 || status >= 300) throw new IllegalStateException("AI endpoint returned HTTP " + status + ": " + response);
        JsonObject envelope = new JsonParser().parse(response.toString()).getAsJsonObject();
        String content = envelope.getAsJsonArray("choices").get(0).getAsJsonObject()
                .getAsJsonObject("message").get("content").getAsString().trim();
        if (content.startsWith("```")) {
            content = content.replaceFirst("^\u0060\u0060\u0060(?:json)?\\s*", "").replaceFirst("\\s*\u0060\u0060\u0060$", "");
        }
        return new JsonParser().parse(content).getAsJsonObject();
    }
}
