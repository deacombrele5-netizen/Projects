package net.zadrot.terraintools;

import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.SidedProxy;
import net.minecraftforge.fml.common.event.FMLPreInitializationEvent;
import net.minecraftforge.fml.common.event.FMLInitializationEvent;
import net.minecraftforge.fml.common.network.NetworkRegistry;
import net.minecraftforge.fml.common.network.simpleimpl.SimpleNetworkWrapper;
import net.minecraftforge.fml.relauncher.Side;
import net.zadrot.terraintools.proxy.CommonProxy;
import net.zadrot.terraintools.network.ToggleMiningMessage;

@Mod(modid=TerrainTools.MODID,name=TerrainTools.NAME,version=TerrainTools.VERSION,acceptedMinecraftVersions="[1.12.2]")
public class TerrainTools {
 public static final String MODID="terraintools", NAME="Terrain Tools", VERSION="1.0.0";
 public static SimpleNetworkWrapper NETWORK;
 @SidedProxy(clientSide="net.zadrot.terraintools.proxy.ClientProxy",serverSide="net.zadrot.terraintools.proxy.CommonProxy") public static CommonProxy PROXY;
 @Mod.EventHandler public void preInit(FMLPreInitializationEvent e){ NETWORK=NetworkRegistry.INSTANCE.newSimpleChannel(MODID); NETWORK.registerMessage(ToggleMiningMessage.Handler.class,ToggleMiningMessage.class,0,Side.SERVER); PROXY.preInit(); }
 @Mod.EventHandler public void init(FMLInitializationEvent e){ PROXY.init(); }
}
