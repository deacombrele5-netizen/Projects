package net.zadrot.livingworld.command;

import com.mojang.brigadier.CommandDispatcher;
import net.minecraft.command.Commands;
import net.minecraft.util.text.StringTextComponent;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.server.ServerWorld;
import net.zadrot.livingworld.city.CityManager;

public final class CityCommands {
    public static void register(CommandDispatcher<net.minecraft.command.CommandSource> dispatcher) {
        dispatcher.register(Commands.literal("lw")
            .then(Commands.literal("start").requires(s -> s.hasPermission(2))
                .executes(ctx -> {
                    ServerWorld world = ctx.getSource().getLevel();
                    BlockPos origin = new BlockPos(ctx.getSource().getPosition());
                    CityManager.start(world, origin);
                    ctx.getSource().sendSuccess(new StringTextComponent("Living World: city development started at " + origin), true);
                    return 1;
                }))
            .then(Commands.literal("stop").requires(s -> s.hasPermission(2))
                .executes(ctx -> {
                    CityManager.stop(ctx.getSource().getLevel());
                    ctx.getSource().sendSuccess(new StringTextComponent("Living World: development paused."), true);
                    return 1;
                }))
            .then(Commands.literal("status")
                .executes(ctx -> {
                    ctx.getSource().sendSuccess(new StringTextComponent(CityManager.status(ctx.getSource().getLevel())), false);
                    return 1;
                }))
        );
    }
    private CityCommands() {}
}
