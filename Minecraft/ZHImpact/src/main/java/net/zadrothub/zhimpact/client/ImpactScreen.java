package net.zadrothub.zhimpact.client;

import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.button.Button;
import net.minecraft.util.text.StringTextComponent;
import net.zadrothub.zhimpact.ImpactConfig;

public class ImpactScreen extends Screen {
    private Button enabledButton;
    private Button multiplierButton;
    private double multiplier;

    public ImpactScreen() {
        super(new StringTextComponent("ZHImpact"));
        this.multiplier = ImpactConfig.MULTIPLIER.get();
    }

    @Override
    protected void init() {
        int cx = this.width / 2;
        int top = this.height / 2 - 65;
        enabledButton = this.addButton(new Button(cx - 100, top, 200, 20,
                enabledText(), button -> {
                    ImpactConfig.ENABLED.set(!ImpactConfig.ENABLED.get());
                    button.setMessage(enabledText());
                }));
        multiplierButton = this.addButton(new Button(cx - 100, top + 28, 200, 20,
                multiplierText(), button -> {
                    // Cycle through safe presets; exact value can be edited in the config file.
                    double[] presets = {0.0D, 0.25D, 1.0D, 2.0D, 10.0D, 100.0D,
                            10000.0D, 1000000.0D, -0.25D, -1.0D, -10.0D, -100.0D};
                    int next = 0;
                    for (int i = 0; i < presets.length; i++) {
                        if (Double.compare(presets[i], multiplier) == 0) {
                            next = (i + 1) % presets.length;
                            break;
                        }
                    }
                    multiplier = presets[next];
                    ImpactConfig.MULTIPLIER.set(multiplier);
                    button.setMessage(multiplierText());
                }));
        this.addButton(new Button(cx - 100, top + 56, 200, 20,
                new StringTextComponent("Open config folder / config file"),
                button -> this.minecraft.displayGuiScreen(new StringTextComponent("Edit config/zhimpact-common.toml"))));
        this.addButton(new Button(cx - 100, top + 88, 200, 20,
                new StringTextComponent("Done"), button -> this.onClose()));
    }

    private StringTextComponent enabledText() {
        return new StringTextComponent("Impact: " + (ImpactConfig.ENABLED.get() ? "ON" : "OFF"));
    }

    private StringTextComponent multiplierText() {
        return new StringTextComponent(String.format(java.util.Locale.ROOT, "Damage multiplier: %.2f", multiplier));
    }

    @Override
    public void render(int mouseX, int mouseY, float partialTicks) {
        this.renderBackground();
        drawCenteredString(this.font, this.title.getString(), this.width / 2, this.height / 2 - 92, 0xFFFFFF);
        drawCenteredString(this.font, "Forge 1.16.5 | U / Г", this.width / 2, this.height / 2 - 78, 0xA0A0A0);
        super.render(mouseX, mouseY, partialTicks);
    }

    @Override
    public boolean isPauseScreen() { return false; }
}