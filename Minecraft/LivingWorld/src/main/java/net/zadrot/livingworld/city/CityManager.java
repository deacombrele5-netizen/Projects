package net.zadrot.livingworld.city;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.tags.BlockTags;
import net.minecraft.entity.merchant.villager.VillagerEntity;
import net.minecraft.entity.item.ItemEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.AxisAlignedBB;
import net.minecraft.world.server.ServerWorld;
import net.minecraftforge.event.TickEvent;

public final class CityManager {
    private static final Map<ServerWorld, Project> PROJECTS = new HashMap<ServerWorld, Project>();
    private static final int BLOCKS_PER_TICK = 24;

    public static void start(ServerWorld world, BlockPos origin) {
        PROJECTS.put(world, new Project(origin));
    }
    public static void stop(ServerWorld world) { PROJECTS.remove(world); }
    public static String status(ServerWorld world) {
        Project p = PROJECTS.get(world);
        if (p == null) return "Living World: no active city project. Use /lw start.";
        return "Living World: " + p.label + " | stage " + (p.stage + 1) + "/4 | blocks " + p.cursor + "/" + p.blocks.size()
            + " | workers " + p.workers + " | stored items " + p.storedItems;
    }

    public static void onWorldTick(TickEvent.WorldTickEvent event) {
        if (event.phase != TickEvent.Phase.END || !(event.world instanceof ServerWorld)) return;
        ServerWorld world = (ServerWorld) event.world;
        Project p = PROJECTS.get(world);
        if (p == null || p.complete || world.getGameTime() % 2 != 0) return;
        p.workVillagers(world);
        int placed = 0;
        while (placed < BLOCKS_PER_TICK && p.cursor < p.blocks.size()) {
            BuildBlock b = p.blocks.get(p.cursor++);
            if (world.isEmptyBlock(b.pos) || world.getBlockState(b.pos).getMaterial().isReplaceable())
                world.setBlock(b.pos, b.state, 3);
            placed++;
        }
        if (p.cursor >= p.blocks.size()) {
            p.stage++;
            if (p.stage < 4) p.planNextStage();
            else {
                p.complete = true;
                p.label = "Initial city plan complete";
            }
        }
    }

    private static final class Project {
        final BlockPos origin;
        final List<BuildBlock> blocks = new ArrayList<BuildBlock>();
        int cursor = 0, stage = 0, workers = 0, storedItems = 0;
        long lastVillagerWork = 0;
        boolean complete = false;
        String label = "Planning";
        Project(BlockPos origin) { this.origin = origin; planNextStage(); }

        void planNextStage() {
            blocks.clear(); cursor = 0;
            if (stage == 0) { label = "Roads and sidewalks"; planRoads(); }
            else if (stage == 1) { label = "Homes and town square"; planHomes(); }
            else if (stage == 2) { label = "Civic centre, shops and pool"; planCivic(); }
            else if (stage == 3) { label = "High-rise landmark"; planTower(); }
        }
        void add(int x, int y, int z, Block block) {
            blocks.add(new BuildBlock(origin.offset(x, y, z), block.defaultBlockState()));
        }
        void fill(int x1,int y1,int z1,int x2,int y2,int z2,Block block) {
            for(int x=x1;x<=x2;x++) for(int y=y1;y<=y2;y++) for(int z=z1;z<=z2;z++) add(x,y,z,block);
        }
        void planRoads() {
            for(int z=-36;z<=36;z++) for(int x=-2;x<=2;x++) {
                add(x,0,z, (x==0 || x==1) ? Blocks.GRAY_CONCRETE : Blocks.STONE_BRICKS);
                if (x==0 && z%8==0) add(x,1,z,Blocks.YELLOW_CONCRETE);
            }
            for(int x=-36;x<=36;x++) for(int z=-2;z<=2;z++)
                add(x,0,z, (z==0 || z==1) ? Blocks.GRAY_CONCRETE : Blocks.STONE_BRICKS);
            for(int z=-34;z<=34;z+=8) {
                add(-4,1,z,Blocks.LANTERN); add(4,1,z,Blocks.LANTERN);
            }
        }
        void building(int cx,int cz,int w,int d,int h,Block wall,Block trim,boolean glass) {
            int x1=cx-w/2, x2=cx+w/2, z1=cz-d/2, z2=cz+d/2;
            fill(x1,0,z1,x2,0,z2,Blocks.SMOOTH_STONE);
            for(int y=1;y<=h;y++) {
                for(int x=x1;x<=x2;x++) for(int z=z1;z<=z2;z++) {
                    boolean edge=x==x1||x==x2||z==z1||z==z2;
                    if(edge) {
                        boolean window=glass && y>1 && y<h && (y%3==0);
                        add(x,y,z,window?Blocks.GLASS:wall);
                    }
                }
                if(y%3==0 && y<h) fill(x1,y,z1,x2,y,z2,trim);
            }
            fill(x1,h+1,z1,x2,h+1,z2,trim);
            add(cx,1,z1,Blocks.AIR); add(cx,2,z1,Blocks.AIR);
        }
        void planHomes() {
            building(-14,-15,11,10,6,Blocks.WHITE_CONCRETE,Blocks.QUARTZ_BLOCK,true);
            building(14,-15,11,10,6,Blocks.LIGHT_GRAY_CONCRETE,Blocks.QUARTZ_BLOCK,true);
            building(-14,15,12,10,7,Blocks.BRICKS,Blocks.DARK_OAK_PLANKS,true);
            building(14,15,12,10,7,Blocks.SPRUCE_PLANKS,Blocks.DARK_OAK_LOG,true);
            fill(-5,0,-5,5,0,5,Blocks.GRASS_BLOCK);
            for(int x=-4;x<=4;x+=2) for(int z=-4;z<=4;z+=2) add(x,1,z,Blocks.FLOWER_POT);
        }
        void planCivic() {
            building(0,-24,18,14,10,Blocks.LIGHT_GRAY_CONCRETE,Blocks.CYAN_STAINED_GLASS,true);
            building(-25,0,10,12,5,Blocks.ORANGE_TERRACOTTA,Blocks.WHITE_CONCRETE,true);
            building(25,0,10,12,5,Blocks.YELLOW_TERRACOTTA,Blocks.WHITE_CONCRETE,true);
            fill(-7,0,22,7,0,34,Blocks.WATER);
            fill(-8,0,21,8,0,35,Blocks.QUARTZ_BLOCK);
        }
        void planTower() {
            building(0,26,17,17,32,Blocks.GRAY_CONCRETE,Blocks.LIGHT_BLUE_STAINED_GLASS,true);
            for(int y=34;y<=38;y++) fill(-2,y,24,2,y,28,Blocks.QUARTZ_BLOCK);
        }

        void workVillagers(ServerWorld world) {
            if(world.getGameTime()-lastVillagerWork<20) return;
            lastVillagerWork=world.getGameTime();
            workers=0; storedItems=0;
            AxisAlignedBB cityArea=new AxisAlignedBB(origin).inflate(48,24,48);
            for(VillagerEntity villager:world.getEntitiesOfClass(VillagerEntity.class,cityArea,v->!v.isBaby())) {
                workers++;
                WorkerInventory inventory=WorkerInventory.get(villager);
                storedItems+=inventory.countItems();
                if(villager.getMainHandItem().isEmpty()) {
                    villager.setItemSlot(net.minecraft.inventory.EquipmentSlotType.MAINHAND,
                        new ItemStack(Items.IRON_AXE));
                }
                BlockPos target=findResource(world,villager.blockPosition());
                if(target!=null) {
                    boolean stone=world.getBlockState(target).getMaterial()==net.minecraft.block.material.Material.STONE;
                    net.minecraft.item.Item wantedTool=stone?Items.IRON_PICKAXE:Items.IRON_AXE;
                    if(villager.getMainHandItem().getItem()!=wantedTool)
                        villager.setItemSlot(net.minecraft.inventory.EquipmentSlotType.MAINHAND,new ItemStack(wantedTool));
                    double distance=villager.distanceToSqr(target.getX()+0.5,target.getY(),target.getZ()+0.5);
                    if(distance>9.0) villager.getNavigation().moveTo(target.getX()+0.5,target.getY(),target.getZ()+0.5,0.8);
                    else {
                        villager.swing(net.minecraft.util.Hand.MAIN_HAND);
                        if(world.getGameTime()%40==0) {
                            boolean wasStone=world.getBlockState(target).getMaterial()==net.minecraft.block.material.Material.STONE;
                            if(world.destroyBlock(target,true,villager)) {
                                if(wasStone) world.addFreshEntity(new ItemEntity(world,target.getX()+0.5,target.getY()+0.5,target.getZ()+0.5,new ItemStack(Items.COBBLESTONE)));
                                for(ItemEntity drop:world.getEntitiesOfClass(ItemEntity.class,new AxisAlignedBB(target).inflate(2.5),e->true)) {
                                    inventory.insert(drop.getItem());
                                    drop.remove();
                                }
                            }
                        }
                    }
                } else {
                    int offset=(int)(villager.getUUID().getLeastSignificantBits()%17L);
                    BlockPos workSite=origin.offset(24+offset,0,(workers%2==0?22:-22));
                    villager.getNavigation().moveTo(workSite.getX()+0.5,workSite.getY(),workSite.getZ()+0.5,0.65);
                }
            }
        }
        BlockPos findResource(ServerWorld world,BlockPos around) {
            for(BlockPos pos:BlockPos.betweenClosed(around.offset(-5,-2,-5),around.offset(5,3,5))) {
                BlockState state=world.getBlockState(pos);
                if(state.is(BlockTags.LOGS)) return pos.immutable();
                if(state.getMaterial()==net.minecraft.block.material.Material.STONE
                    && pos.getY()>=origin.getY()-1
                    && Math.abs(pos.getX()-origin.getX())>20
                    && Math.abs(pos.getZ()-origin.getZ())>20) return pos.immutable();
            }
            return null;
        }
    }

    private static final class WorkerInventory {
        private static final Map<java.util.UUID,WorkerInventory> DATA=new HashMap<java.util.UUID,WorkerInventory>();
        private final List<ItemStack> items=new ArrayList<ItemStack>();
        static WorkerInventory get(VillagerEntity villager) {
            WorkerInventory data=DATA.get(villager.getUUID());
            if(data==null) { data=new WorkerInventory(); DATA.put(villager.getUUID(),data); }
            return data;
        }
        int insert(ItemStack incoming) {
            if(incoming.isEmpty() || items.size()>=16) return 0;
            int accepted=Math.min(incoming.getCount(),incoming.getMaxStackSize());
            ItemStack copy=incoming.copy(); copy.setCount(accepted);
            items.add(copy);
            incoming.shrink(accepted);
            return accepted;
        }
        int countItems() {
            int total=0;
            for(ItemStack stack:items) total+=stack.getCount();
            return total;
        }
    }
    public static boolean giveItem(VillagerEntity villager,ItemStack held) {
        return WorkerInventory.get(villager).insert(held)>0;
    }
    private static final class BuildBlock {
        final BlockPos pos; final BlockState state;
        BuildBlock(BlockPos pos, BlockState state) { this.pos=pos; this.state=state; }
    }
    private CityManager() {}
}
