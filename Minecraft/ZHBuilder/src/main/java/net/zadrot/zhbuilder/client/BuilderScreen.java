package net.zadrot.zhbuilder.client;

import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.util.text.StringTextComponent;

public class BuilderScreen extends Screen {
    private TextFieldWidget prompt;

    public BuilderScreen() {
        super(new StringTextComponent("ZH Builder"));
    }

    @Override
    protected void init() {
        int center = this.width / 2;
        prompt = new TextFieldWidget(this.font, center - 150, this.height / 2 - 12, 300, 24, new StringTextComponent("Describe a structure"));
        prompt.setMaxLength(500);
        prompt.setSuggestion("e.g. modern house with glass walls and a pool");
        this.addWidget(prompt);
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center - 150, this.height / 2 + 24, 145, 20,
                new StringTextComponent("Generate"), button -> send("/zhb generate " + prompt.getValue())));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center + 5, this.height / 2 + 24, 145, 20,
                new StringTextComponent("Close"), button -> this.onClose()));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center - 150, this.height / 2 + 52, 95, 20,
                new StringTextComponent("Set Pos 1"), button -> send("/zhb pos1")));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center - 50, this.height / 2 + 52, 95, 20,
                new StringTextComponent("Set Pos 2"), button -> send("/zhb pos2")));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center + 50, this.height / 2 + 52, 100, 20,
                new StringTextComponent("Copy"), button -> send("/zhb copy")));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center - 150, this.height / 2 + 78, 145, 20,
                new StringTextComponent("Paste"), button -> send("/zhb paste")));
        this.addButton(new net.minecraft.client.gui.widget.button.Button(center + 5, this.height / 2 + 78, 145, 20,
                new StringTextComponent("Help in chat"), button -> send("/zhb help")));
    }

    private void send(String command) {
        if (this.minecraft != null && this.minecraft.player != null) {
            this.minecraft.player.chat(command);
            this.onClose();
        }
    }

    @Override
    public void tick() {
        if (prompt != null) prompt.tick();
    }

    @Override
    public boolean charTyped(char character, int modifiers) {
        return prompt != null && prompt.charTyped(character, modifiers) || super.charTyped(character, modifiers);
    }

    @Override
    public boolean keyPressed(int keyCode, int scanCode, int modifiers) {
        if (prompt != null && prompt.keyPressed(keyCode, scanCode, modifiers)) return true;
        return super.keyPressed(keyCode, scanCode, modifiers);
    }

    @Override
    public void render(net.minecraft.client.gui.matrixstack.MatrixStack matrixStack, int mouseX, int mouseY, float partialTicks) {
        this.renderBackground(matrixStack);
        drawCenteredString(matrixStack, this.font, "ZH BUILDER", this.width / 2, this.height / 2 - 58, 0x55D6FF);
        drawCenteredString(matrixStack, this.font, "AI generation + server-side copy/paste", this.width / 2, this.height / 2 - 40, 0xFFFFFF);
        if (prompt != null) prompt.render(matrixStack, mouseX, mouseY, partialTicks);
        super.render(matrixStack, mouseX, mouseY, partialTicks);
    }
}
