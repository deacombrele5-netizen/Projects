package net.zadrothub.zhimpact;

import net.minecraftforge.common.ForgeConfigSpec;

public final class ImpactConfig {
    private static final ForgeConfigSpec.Builder BUILDER = new ForgeConfigSpec.Builder();
    public static final ForgeConfigSpec.DoubleValue MULTIPLIER;
    public static final ForgeConfigSpec.BooleanValue ENABLED;
    public static final ForgeConfigSpec SPEC;

    // Deliberately finite: unbounded/infinite values can destabilize Minecraft's damage,
    // health and attribute calculations. This cap is for controlled test environments.
    public static final double MAX_MULTIPLIER = 1.0E9D;

    static {
        BUILDER.push("impact");
        ENABLED = BUILDER.comment("Enable the server-authoritative damage laboratory.")
                .define("enabled", false);
        MULTIPLIER = BUILDER.comment("Damage multiplier. Negative values reflect damage to the attacker.")
                .defineInRange("multiplier", 1.0D, -MAX_MULTIPLIER, MAX_MULTIPLIER);
        BUILDER.pop();
        SPEC = BUILDER.build();
    }

    private ImpactConfig() {}
}