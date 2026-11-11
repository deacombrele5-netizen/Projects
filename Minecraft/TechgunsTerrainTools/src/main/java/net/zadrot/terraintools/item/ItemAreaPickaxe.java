package net.zadrot.terraintools.item;
import net.minecraft.block.state.IBlockState;
import net.minecraft.entity.EntityLivingBase;
import net.minecraft.entity.player.EntityPlayerMP;
import net.minecraft.item.ItemPickaxe;
import net.minecraft.item.ItemStack;
import net.minecraft.tileentity.TileEntity;
import net.minecraft.util.EnumFacing;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.World;
import net.zadrot.terraintools.network.ToggleMiningMessage;
public class ItemAreaPickaxe extends ItemPickaxe {
 public ItemAreaPickaxe(ToolMaterial m){super(m);setRegistryName("area_pickaxe");setUnlocalizedName("area_pickaxe");setMaxStackSize(1);}
 @Override public boolean onBlockDestroyed(ItemStack stack,World world,IBlockState state,BlockPos pos,EntityLivingBase entity){
  boolean result=super.onBlockDestroyed(stack,world,state,pos,entity);
  if(world.isRemote||!(entity instanceof EntityPlayerMP))return result;
  EntityPlayerMP player=(EntityPlayerMP)entity;
  if(!ToggleMiningMessage.Handler.ENABLED.contains(player.getUniqueID()))return result;
  float hardness=state.getBlockHardness(world,pos); if(hardness<0||hardness>4.0F)return result;
  EnumFacing.Axis axis=player.getHorizontalFacing().getAxis();
  for(int a=-1;a<=1;a++)for(int b=-1;b<=1;b++){
   if(a==0&&b==0)continue;
   BlockPos target=axis==EnumFacing.Axis.Z?pos.add(a,b,0):pos.add(0,b,a);
   if(!world.isBlockLoaded(target))continue;
   IBlockState s=world.getBlockState(target); float h=s.getBlockHardness(world,target);
   if(h<0||h>4.0F||world.getTileEntity(target)!=null||!s.getBlock().canHarvestBlock(world,target,player))continue;
   TileEntity te=null; s.getBlock().harvestBlock(world,player,target,s,te,stack); world.setBlockToAir(target); stack.damageItem(1,player);
   if(stack.isEmpty())return result;
  }
  return result;
 }
}
