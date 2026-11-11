package net.zadrot.terraintools.network;
import io.netty.buffer.ByteBuf;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import net.minecraft.entity.player.EntityPlayerMP;
import net.minecraftforge.fml.common.network.simpleimpl.IMessage;
import net.minecraftforge.fml.common.network.simpleimpl.IMessageHandler;
import net.minecraftforge.fml.common.network.simpleimpl.MessageContext;
public class ToggleMiningMessage implements IMessage {
 private boolean enabled;
 public ToggleMiningMessage(){} public ToggleMiningMessage(boolean value){enabled=value;}
 public void fromBytes(ByteBuf b){enabled=b.readBoolean();} public void toBytes(ByteBuf b){b.writeBoolean(enabled);}
 public static class Handler implements IMessageHandler<ToggleMiningMessage,IMessage>{
  public static final Set<UUID> ENABLED=ConcurrentHashMap.newKeySet();
  public IMessage onMessage(ToggleMiningMessage m,MessageContext c){ final EntityPlayerMP p=c.getServerHandler().player; p.getServerWorld().addScheduledTask(()->{if(m.enabled)ENABLED.add(p.getUniqueID());else ENABLED.remove(p.getUniqueID());}); return null; }
 }
}
