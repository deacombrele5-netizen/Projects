package net.zadrot.terraintools.proxy;

import net.minecraft.client.Minecraft;
import net.minecraft.client.settings.KeyBinding;
import net.minecraft.util.text.TextComponentTranslation;
import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.client.registry.ClientRegistry;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.InputEvent;
import net.zadrot.terraintools.TerrainTools;
import net.zadrot.terraintools.network.ToggleMiningMessage;
import org.lwjgl.input.Keyboard;

public class ClientProxy extends CommonProxy {
    private KeyBinding toggle;
    private boolean enabled = false;

    @Override
    public void init() {
        toggle = new KeyBinding(
                "key.terraintools.toggle",
                Keyboard.KEY_G,
                "key.categories.terraintools"
        );
        ClientRegistry.registerKeyBinding(toggle);
        MinecraftForge.EVENT_BUS.register(this);
    }

    @SubscribeEvent
    public void onKey(InputEvent.KeyInputEvent event) {
        while (toggle.isPressed()) {
            Minecraft minecraft = Minecraft.getMinecraft();
            if (minecraft.player == null || minecraft.world == null) {
                continue;
            }

            enabled = !enabled;
            TerrainTools.NETWORK.sendToServer(new ToggleMiningMessage(enabled));
            minecraft.player.sendStatusMessage(
                    new TextComponentTranslation(
                            enabled ? "message.terraintools.enabled" : "message.terraintools.disabled"
                    ),
                    true
            );
        }
    }
}
