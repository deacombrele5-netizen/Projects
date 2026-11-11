package net.zadrothub.zhimpact;

import net.minecraft.entity.LivingEntity;
import net.minecraft.util.DamageSource;
import net.minecraftforge.event.entity.living.LivingHurtEvent;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.fml.common.Mod;

/** Server-authoritative damage adjustment for worlds/servers running this mod. */
@Mod.EventBusSubscriber(modid = ZHImpact.MOD_ID)
public final class DamageController {
    private static final float MAX_APPLIED_DAMAGE = 1.0E9F;

    @SubscribeEvent
    public static void onLivingHurt(LivingHurtEvent event) {
        if (event.getEntityLiving().world.isRemote || !ImpactConfig.ENABLED.get()) return;

        double multiplier = ImpactConfig.MULTIPLIER.get();
        if (multiplier == 0.0D) {
            event.setCanceled(true);
            return;
        }

        if (multiplier > 0.0D) {
            double adjusted = (double) event.getAmount() * multiplier;
            event.setAmount((float) Math.min(adjusted, MAX_APPLIED_DAMAGE));
            return;
        }

        // Negative mode reflects damage to the living attacker, on the authoritative server.
        DamageSource source = event.getSource();
        if (source.getTrueSource() instanceof LivingEntity) {
            LivingEntity attacker = (LivingEntity) source.getTrueSource();
            float reflected = (float) Math.min(
                    Math.abs((double) event.getAmount() * multiplier), MAX_APPLIED_DAMAGE);
            event.setCanceled(true);
            if (reflected > 0.0F && attacker.isAlive()) {
                attacker.attackEntityFrom(DamageSource.GENERIC, reflected);
            }
        } else {
            event.setCanceled(true);
        }
    }
}