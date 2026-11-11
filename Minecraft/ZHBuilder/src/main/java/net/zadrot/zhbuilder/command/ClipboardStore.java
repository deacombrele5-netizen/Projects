package net.zadrot.zhbuilder.command;

import net.minecraft.block.BlockState;
import net.minecraft.nbt.CompoundNBT;
import net.minecraft.nbt.ListNBT;
import net.minecraft.nbt.NBTUtil;
import net.minecraft.tileentity.TileEntity;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.server.ServerWorld;

import java.util.ArrayList;
import java.util.List;

final class ClipboardStore {
    static final class Entry {
        final int x, y, z;
        final BlockState state;
        final CompoundNBT tileData;

        Entry(int x, int y, int z, BlockState state, CompoundNBT tileData) {
            this.x = x; this.y = y; this.z = z;
            this.state = state; this.tileData = tileData;
        }
    }

    final int sizeX, sizeY, sizeZ;
    final List<Entry> entries;

    ClipboardStore(int sizeX, int sizeY, int sizeZ, List<Entry> entries) {
        this.sizeX = sizeX; this.sizeY = sizeY; this.sizeZ = sizeZ;
        this.entries = entries;
    }

    static ClipboardStore capture(ServerWorld world, BlockPos min, BlockPos max, int limit) {
        int sx = max.getX() - min.getX() + 1;
        int sy = max.getY() - min.getY() + 1;
        int sz = max.getZ() - min.getZ() + 1;
        long volume = (long) sx * sy * sz;
        if (volume > limit) throw new IllegalArgumentException("Selection is " + volume + " blocks; limit is " + limit + ".");
        List<Entry> result = new ArrayList<>();
        for (int y = min.getY(); y <= max.getY(); y++) {
            for (int z = min.getZ(); z <= max.getZ(); z++) {
                for (int x = min.getX(); x <= max.getX(); x++) {
                    BlockPos pos = new BlockPos(x, y, z);
                    BlockState state = world.getBlockState(pos);
                    CompoundNBT tile = null;
                    TileEntity entity = world.getBlockEntity(pos);
                    if (entity != null) tile = entity.save(new CompoundNBT());
                    result.add(new Entry(x - min.getX(), y - min.getY(), z - min.getZ(), state, tile));
                }
            }
        }
        return new ClipboardStore(sx, sy, sz, result);
    }

    void paste(ServerWorld world, BlockPos origin) {
        for (Entry entry : entries) {
            BlockPos target = origin.offset(entry.x, entry.y, entry.z);
            if (world.isOutsideBuildHeight(target)) continue;
            world.setBlock(target, entry.state, 3);
            if (entry.tileData != null) {
                TileEntity tile = world.getBlockEntity(target);
                if (tile != null) {
                    CompoundNBT data = entry.tileData.copy();
                    data.putInt("x", target.getX());
                    data.putInt("y", target.getY());
                    data.putInt("z", target.getZ());
                    tile.load(entry.state, data);
                    tile.setChanged();
                }
            }
        }
    }

    CompoundNBT toNbt() {
        CompoundNBT root = new CompoundNBT();
        root.putInt("sizeX", sizeX);
        root.putInt("sizeY", sizeY);
        root.putInt("sizeZ", sizeZ);
        ListNBT list = new ListNBT();
        for (Entry entry : entries) {
            CompoundNBT tag = new CompoundNBT();
            tag.putInt("x", entry.x); tag.putInt("y", entry.y); tag.putInt("z", entry.z);
            tag.put("state", NBTUtil.writeBlockState(entry.state));
            if (entry.tileData != null) tag.put("tile", entry.tileData);
            list.add(tag);
        }
        root.put("blocks", list);
        return root;
    }

    static ClipboardStore fromNbt(CompoundNBT root) {
        int sx = root.getInt("sizeX"), sy = root.getInt("sizeY"), sz = root.getInt("sizeZ");
        List<Entry> result = new ArrayList<>();
        ListNBT list = root.getList("blocks", 10);
        for (int i = 0; i < list.size(); i++) {
            CompoundNBT tag = list.getCompound(i);
            BlockState state = NBTUtil.readBlockState(tag.getCompound("state"));
            CompoundNBT tile = tag.contains("tile", 10) ? tag.getCompound("tile").copy() : null;
            result.add(new Entry(tag.getInt("x"), tag.getInt("y"), tag.getInt("z"), state, tile));
        }
        return new ClipboardStore(sx, sy, sz, result);
    }
}
