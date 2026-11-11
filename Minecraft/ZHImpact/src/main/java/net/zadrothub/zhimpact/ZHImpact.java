package net.zadrothub.zhimpact;

import net.minecraftforge.fml.ModLoadingContext;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.config.ModConfig;

@Mod(ZHImpact.MOD_ID)
public class ZHImpact {
    public static final String MOD_ID = "zhimpact";

    public ZHImpact() {
        ModLoadingContext.get().registerConfig(ModConfig.Type.COMMON, ImpactConfig.SPEC);
    }
}
