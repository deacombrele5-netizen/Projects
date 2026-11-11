package net.zadrot.terraintools.item;
import net.minecraft.creativetab.CreativeTabs;
import net.minecraft.item.Item;
import net.minecraftforge.common.util.EnumHelper;
import net.minecraftforge.event.RegistryEvent;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
@Mod.EventBusSubscriber(modid="terraintools")
public final class ModItems {
 public static final Item.ToolMaterial MATERIAL=EnumHelper.addToolMaterial("TERRAIN_TOOLS",2,900,7.0F,2.0F,12);
 public static final ItemAreaPickaxe AREA_PICKAXE=(ItemAreaPickaxe)new ItemAreaPickaxe(MATERIAL).setCreativeTab(CreativeTabs.TOOLS);
 private ModItems(){}
 @SubscribeEvent public static void register(RegistryEvent.Register<Item> e){e.getRegistry().register(AREA_PICKAXE);}
}
