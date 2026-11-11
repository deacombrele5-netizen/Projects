package net.zadrot.zhbuilder.client;

import net.minecraft.client.Minecraft;
import net.minecraft.client.settings.KeyBinding;
import net.minecraftforge.api.distmarker.Dist;
import net.minecraftforge.client.event.InputEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.fml.client.registry.ClientRegistry;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.event.lifecycle.FMLClientSetupEvent;
import org.lwjgl.glfw.GLFW;
import net.zadrot.zhbuilder.ZHBuilder;

@Mod.EventBusSubscriber(modid = ZHBuilder.MOD_ID, value = Dist.CLIENT, bus = Mod.EventBusSubscriber.Bus.MOD)
public final class ClientEvents {
    public static final KeyBinding OPEN_MENU = new KeyBinding("key.zhbuilder.open", GLFW.GLFW_KEY_U, "key.categories.zhbuilder");

    private ClientEvents() {}

    @SubscribeEvent
    public static void onClientSetup(FMLClientSetupEvent event) {
        event.enqueueWork(() -> ClientRegistry.registerKeyBinding(OPEN_MENU));
    }

    @Mod.EventBusSubscriber(modid = ZHBuilder.MOD_ID, value = Dist.CLIENT)
    public static final class InputEvents {
        @SubscribeEvent
        public static void onKeyInput(InputEvent.KeyInputEvent event) {
            Minecraft minecraft = Minecraft.getInstance();
            while (OPEN_MENU.isPressed()) {
                if (minecraft.screen == null) minecraft.setScreen(new BuilderScreen());
            }
        }
    }
}
