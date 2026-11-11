package net.zadrothub.zhimpact;

import net.minecraftforge.common.ForgeConfigSpec;

/**
 * Stores values for the local configuration UI only.
 * This configuration does not modify entity damage or combat behavior.
 */
public final class ImpactConfig {
    private static final ForgeConfigSpec.Builder BUILDER = new ForgeConfigSpec.Builder();
    public static final ForgeConfigSpec.DoubleValue MULTIPLIER;
    public static final ForgeConfigSpec.BooleanValue ENABLED;
    public static final ForgeConfigSpec SPEC;

    public static final double MAX_MULTIPLIER = 1.0E9D;

    static {
        BUILDER.push("impact");
        ENABLED = BUILDER.comment("UI preview toggle only; does not change gameplay or entity damage.")
                .define("enabled", false);
        MULTIPLIER = BUILDER.comment("UI preview value only; does not change gameplay or entity damage.")
                .defineInRange("multiplier", 1.0D, -MAX_MULTIPLIER, MAX_MULTIPLIER);
        BUILDER.pop();
        SPEC = BUILDER.build();
    }

    private ImpactConfig() {}
}
