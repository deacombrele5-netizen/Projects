package net.zadrothub.zhimpact.client;

import net.minecraft.client.Minecraft;
import net.minecraft.client.settings.KeyBinding;
import net.minecraft.client.util.InputMappings;
import net.minecraftforge.client.event.InputEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.api.distmarker.Dist;

@Mod.EventBusSubscriber(modid = "zhimpact", value = Dist.CLIENT)
public final class ClientEvents {
    public static final KeyBinding OPEN_MENU = new KeyBinding(
            "key.zhimpact.open_menu", InputMappings.getKey("key.keyboard.u"),
            "key.categories.zhimpact");

    @SubscribeEvent
    public static void onKeyInput(InputEvent.KeyInputEvent event) {
        Minecraft mc = Minecraft.getInstance();
        while (OPEN_MENU.isPressed()) {
            if (mc.currentScreen == null) mc.displayGuiScreen(new ImpactScreen());
            else if (mc.currentScreen instanceof ImpactScreen) mc.displayGuiScreen(null);
        }
    }
}