package net.zadrot.terraintools.network;

import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.PlayerEvent;

/** Keeps the server-side toggle set free of players who have left the server. */
@Mod.EventBusSubscriber(modid = "terraintools")
public final class ServerEvents {
    private ServerEvents() {
    }

    @SubscribeEvent
    public static void onPlayerLogout(PlayerEvent.PlayerLoggedOutEvent event) {
        ToggleMiningMessage.Handler.ENABLED.remove(event.player.getUniqueID());
    }
}
