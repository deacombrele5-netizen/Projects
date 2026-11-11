package net.zadrot.terraintools.item;

import net.minecraft.block.state.IBlockState;
import net.minecraft.entity.EntityLivingBase;
import net.minecraft.entity.player.EntityPlayerMP;
import net.minecraft.item.ItemPickaxe;
import net.minecraft.item.ItemStack;
import net.minecraft.util.EnumFacing;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.World;
import net.zadrot.terraintools.network.ToggleMiningMessage;

/**
 * A bounded area-mining pickaxe. Extra blocks are harvested through the
 * normal server interaction manager so Forge break events and protection
 * plugins/mods can approve or reject each block.
 */
public class ItemAreaPickaxe extends ItemPickaxe {
    private static final ThreadLocal<Boolean> AREA_BREAK_IN_PROGRESS =
            ThreadLocal.withInitial(() -> Boolean.FALSE);

    public ItemAreaPickaxe(ToolMaterial material) {
        super(material);
        setRegistryName("area_pickaxe");
        setUnlocalizedName("area_pickaxe");
        setMaxStackSize(1);
    }

    @Override
    public boolean onBlockDestroyed(ItemStack stack, World world, IBlockState state,
                                    BlockPos pos, EntityLivingBase entity) {
        boolean result = super.onBlockDestroyed(stack, world, state, pos, entity);

        // The nested calls made by tryHarvestBlock must not start another 3x3 pass.
        if (AREA_BREAK_IN_PROGRESS.get() || world.isRemote || !(entity instanceof EntityPlayerMP)) {
            return result;
        }

        EntityPlayerMP player = (EntityPlayerMP) entity;
        if (!ToggleMiningMessage.Handler.ENABLED.contains(player.getUniqueID())) {
            return result;
        }

        float centerHardness = state.getBlockHardness(world, pos);
        if (centerHardness < 0.0F || centerHardness > 4.0F) {
            return result;
        }

        EnumFacing.Axis axis = player.getHorizontalFacing().getAxis();
        AREA_BREAK_IN_PROGRESS.set(Boolean.TRUE);
        try {
            for (int a = -1; a <= 1; a++) {
                for (int b = -1; b <= 1; b++) {
                    if (a == 0 && b == 0) {
                        continue;
                    }

                    BlockPos target = axis == EnumFacing.Axis.Z
                            ? pos.add(a, b, 0)
                            : pos.add(0, b, a);

                    if (!world.isBlockLoaded(target) || world.getTileEntity(target) != null) {
                        continue;
                    }

                    IBlockState targetState = world.getBlockState(target);
                    float hardness = targetState.getBlockHardness(world, target);
                    if (hardness < 0.0F || hardness > 4.0F) {
                        continue;
                    }

                    // Uses vanilla/Forge harvesting, including break events,
                    // permission checks, drops, and normal tool durability.
                    player.interactionManager.tryHarvestBlock(target);

                    if (stack.isEmpty()) {
                        return result;
                    }
                }
            }
        } finally {
            AREA_BREAK_IN_PROGRESS.remove();
        }

        return result;
    }
}
