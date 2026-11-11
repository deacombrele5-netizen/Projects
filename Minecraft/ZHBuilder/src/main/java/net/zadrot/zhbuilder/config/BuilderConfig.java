package net.zadrot.zhbuilder.config;

import net.minecraftforge.common.ForgeConfigSpec;

public final class BuilderConfig {
    private static final ForgeConfigSpec.Builder BUILDER = new ForgeConfigSpec.Builder();

    public static final ForgeConfigSpec.BooleanValue AI_ENABLED;
    public static final ForgeConfigSpec.ConfigValue<String> AI_ENDPOINT;
    public static final ForgeConfigSpec.ConfigValue<String> AI_MODEL;
    public static final ForgeConfigSpec.ConfigValue<String> AI_API_KEY;
    public static final ForgeConfigSpec.IntValue MAX_COPY_BLOCKS;
    public static final ForgeConfigSpec.IntValue MAX_AI_BLOCKS;
    public static final ForgeConfigSpec SPEC;

    static {
        BUILDER.push("general");
        MAX_COPY_BLOCKS = BUILDER.comment("Maximum blocks in a copied selection.")
                .defineInRange("maxCopyBlocks", 32768, 1, 262144);
        MAX_AI_BLOCKS = BUILDER.comment("Maximum blocks an AI generation request may place.")
                .defineInRange("maxAiBlocks", 8192, 1, 32768);
        BUILDER.pop();

        BUILDER.push("ai");
        AI_ENABLED = BUILDER.comment("Enable remote AI generation. Copy/paste works without this.")
                .define("aiEnabled", false);
        AI_ENDPOINT = BUILDER.comment("OpenAI-compatible chat completions endpoint.")
                .define("aiEndpoint", "https://api.openai.com/v1/chat/completions");
        AI_MODEL = BUILDER.comment("Model name accepted by the configured endpoint.")
                .define("aiModel", "gpt-4o-mini");
        AI_API_KEY = BUILDER.comment("Private API key. Do not share this file or commit the key.")
                .define("aiApiKey", "");
        BUILDER.pop();
        SPEC = BUILDER.build();
    }

    private BuilderConfig() {}
}
