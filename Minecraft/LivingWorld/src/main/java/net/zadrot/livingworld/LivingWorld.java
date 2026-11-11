package net.zadrot.livingworld;

import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.event.RegisterCommandsEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.zadrot.livingworld.command.CityCommands;
import net.zadrot.livingworld.city.CityManager;

@Mod(LivingWorld.MOD_ID)
public class LivingWorld {
    public static final String MOD_ID = "livingworld";
    public LivingWorld() {
        MinecraftForge.EVENT_BUS.register(this);
        MinecraftForge.EVENT_BUS.addListener(CityManager::onWorldTick);
    }
    @SubscribeEvent
    public void onRegisterCommands(RegisterCommandsEvent event) {
        CityCommands.register(event.getDispatcher());
    }
}
