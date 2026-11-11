package net.zadrot.livingworld;

import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.event.RegisterCommandsEvent;
import net.minecraftforge.event.entity.player.PlayerInteractEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraft.entity.merchant.villager.VillagerEntity;
import net.minecraft.util.text.StringTextComponent;
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
    @SubscribeEvent
    public void onGiveItemToVillager(PlayerInteractEvent.EntityInteract event) {
        if (event.getHand() != net.minecraft.util.Hand.MAIN_HAND) return;
        if (!(event.getTarget() instanceof VillagerEntity)) return;
        if (event.getItemStack().isEmpty()) return;
        VillagerEntity villager=(VillagerEntity)event.getTarget();
        if (CityManager.giveItem(villager,event.getItemStack())) {
            event.getPlayer().displayClientMessage(
                new StringTextComponent("Living World: предмет передан жителю."), false);
            event.setCanceled(true);
        }
    }
}
