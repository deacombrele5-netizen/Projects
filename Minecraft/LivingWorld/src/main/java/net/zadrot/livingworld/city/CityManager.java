package net.zadrot.livingworld.city;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.entity.Entity;
import net.minecraft.entity.merchant.villager.VillagerEntity;
import net.minecraft.entity.item.ItemEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Direction;
import net.minecraft.util.ResourceLocation;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.vector.Vector3d;
import net.minecraft.world.World;
import net.minecraft.world.server.ServerWorld;
import net.minecraftforge.event.TickEvent;
import net.minecraftforge.registries.ForgeRegistries;

public final class CityManager {
    private static final Map<ServerWorld, Project> PROJECTS = new HashMap<ServerWorld, Project>();
    private static final int BLOCKS_PER_TICK = 24;

    public static void start(ServerWorld world, BlockPos origin) {
        PROJECTS.put(world, new Project(origin));
    }
    public static void stop(ServerWorld world) { PROJECTS.remove(world); }
    public static String status(ServerWorld world) {
        Project p = PROJECTS.get(world);
        return p == null ? "Living World: no active city project. Use /lw start." :
            "Living World: " + p.label + " | stage " + p.stage + " | blocks " + p.cursor + "/" + p.blocks.size();
    }

    public static void onWorldTick(TickEvent.WorldTickEvent event) {
        if (event.phase != TickEvent.Phase.END || !(event.world instanceof ServerWorld)) return;
        ServerWorld world = (ServerWorld) event.world;
        Project p = PROJECTS.get(world);
        if (p == null || world.getGameTime() % 2 != 0) return;
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
            else { p.label = "City foundation complete"; }
        }
    }

    private static final class Project {
        final BlockPos origin;
        final List<BuildBlock> blocks = new ArrayList<BuildBlock>();
        int cursor = 0, stage = 0;
        long lastVillagerWork = 0;
        String label = "Planning roads and first buildings";
        Project(BlockPos origin) { this.origin = origin; planNextStage(); }

        void planNextStage() {
            blocks.clear(); cursor = 0;
            if (stage == 0) { label = "Roads and sidewalks"; planRoads(); }
            else if (stage == 1) { label = "Homes and town square"; planHomes(); }
            else if (stage == 2) { label = "Civic centre and shops"; planCivic(); }
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
            if(world.getGameTime()-lastVillagerWork<40) return;
            lastVillagerWork=world.getGameTime();
            for(VillagerEntity v:world.getEntitiesOfClass(VillagerEntity.class,
                    new net.minecraft.util.math.AxisAlignedBB(origin).inflate(48), e->!e.isBaby())) {
                if(v.getMainHandItem().isEmpty()) {
                    v.setItemSlot(net.minecraft.entity.EquipmentSlotType.MAINHAND,
                        new ItemStack(v.getRandom().nextBoolean()?Items.IRON_PICKAXE:Items.IRON_AXE));
                }
                v.swing(net.minecraft.util.Hand.MAIN_HAND);
                if(v.distanceToSqr(origin.getX()+0.5,origin.getY(),origin.getZ()+0.5)>100)
                    v.getNavigation().moveTo(origin.getX()+0.5,origin.getY(),origin.getZ()+0.5,0.7);
            }
        }
    }
    private static final class BuildBlock {
        final BlockPos pos; final BlockState state;
        BuildBlock(BlockPos pos, BlockState state) { this.pos=pos; this.state=state; }
    }
    private CityManager() {}
}
