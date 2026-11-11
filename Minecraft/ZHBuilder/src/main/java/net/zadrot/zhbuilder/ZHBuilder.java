package net.zadrot.zhbuilder;

import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.ModLoadingContext;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.config.ModConfig;
import net.zadrot.zhbuilder.command.BuilderCommands;
import net.zadrot.zhbuilder.config.BuilderConfig;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

@Mod(ZHBuilder.MOD_ID)
public class ZHBuilder {
    public static final String MOD_ID = "zhbuilder";
    public static final Logger LOGGER = LogManager.getLogger();

    public ZHBuilder() {
        ModLoadingContext.get().registerConfig(ModConfig.Type.COMMON, BuilderConfig.SPEC);
        MinecraftForge.EVENT_BUS.addListener(BuilderCommands::register);
        LOGGER.info("ZH Builder initialized");
    }
}
