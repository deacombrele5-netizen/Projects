package net.zadrothub.zhimpact;

import net.minecraft.entity.LivingEntity;
import net.minecraft.util.DamageSource;
import net.minecraftforge.event.entity.living.LivingHurtEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.fml.common.Mod;

/**
 * Runs on the logical server. On multiplayer, only the server's installed config
 * controls the result; changing a client screen cannot force a remote server to accept damage.
 */
@Mod.EventBusSubscriber(modid = ZHImpact.MOD_ID)
public final class DamageController {
    @SubscribeEvent
    public static void onLivingHurt(LivingHurtEvent event) {
        if (event.getEntity().world.isRemote || !ImpactConfig.ENABLED.get()) return;

        double multiplier = ImpactConfig.MULTIPLIER.get();
        if (multiplier == 0.0D) {
            event.setCanceled(true);
            return;
        }

        if (multiplier > 0.0D) {
            double adjusted = event.getAmount() * multiplier;
            if (Double.isFinite(adjusted)) {
                event.setAmount((float) Math.min(adjusted, Float.MAX_VALUE));
            } else {
                event.setAmount(Float.MAX_VALUE);
            }
            return;
        }

        // Negative mode: cancel the original hit and reflect a bounded amount
        // back to the living attacker. This is intended for worlds/servers administered
        // by the player, where this mod is installed on the authoritative server.
        DamageSource source = event.getSource();
        if (source.getTrueSource() instanceof LivingEntity) {
            LivingEntity attacker = (LivingEntity) source.getTrueSource();
            float reflected = (float) Math.min(
                    Math.abs((double) event.getAmount() * multiplier), Float.MAX_VALUE);
            event.setCanceled(true);
            if (reflected > 0.0F && attacker.isAlive()) {
                attacker.attackEntityFrom(DamageSource.GENERIC, reflected);
            }
        } else {
            event.setCanceled(true);
        }
    }
}