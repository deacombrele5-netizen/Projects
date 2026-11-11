package net.zadrothub.zhimpact.client;

import com.mojang.blaze3d.matrix.MatrixStack;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.widget.TextFieldWidget;
import net.minecraft.client.gui.widget.button.Button;
import net.minecraft.util.text.TranslationTextComponent;
import net.zadrothub.zhimpact.ImpactConfig;

import java.util.Locale;

public class ImpactScreen extends Screen {
    private double multiplier;
    private TextFieldWidget multiplierInput;
    private String status = "screen.zhimpact.ready";

    public ImpactScreen() {
        super(new TranslationTextComponent("screen.zhimpact.title"));
        this.multiplier = ImpactConfig.MULTIPLIER.get();
    }

    @Override
    protected void init() {
        int cx = this.width / 2;
        int top = this.height / 2 - 88;

        this.addButton(new Button(cx - 100, top, 200, 20, enabledText(), button -> {
            ImpactConfig.ENABLED.set(!ImpactConfig.ENABLED.get());
            button.setMessage(enabledText());
        }));

        multiplierInput = new TextFieldWidget(this.font, cx - 100, top + 28, 200, 20,
                new TranslationTextComponent("screen.zhimpact.multiplier"));
        multiplierInput.setMaxStringLength(32);
        multiplierInput.setText(String.format(Locale.ROOT, "%.6g", multiplier));
        this.addButton(multiplierInput);

        this.addButton(new Button(cx - 100, top + 54, 200, 20,
                new TranslationTextComponent("screen.zhimpact.apply"), button -> applyMultiplier()));

        this.addButton(new Button(cx - 100, top + 82, 200, 20,
                new TranslationTextComponent("screen.zhimpact.preset"), button -> cyclePreset()));

        this.addButton(new Button(cx - 100, top + 112, 200, 20,
                new TranslationTextComponent("screen.zhimpact.done"), button -> this.onClose()));
    }

    private void applyMultiplier() {
        try {
            double parsed = Double.parseDouble(multiplierInput.getText().trim());
            if (!Double.isFinite(parsed)) {
                status = "screen.zhimpact.finite";
                return;
            }
            multiplier = Math.max(-ImpactConfig.MAX_MULTIPLIER,
                    Math.min(ImpactConfig.MAX_MULTIPLIER, parsed));
            ImpactConfig.MULTIPLIER.set(multiplier);
            multiplierInput.setText(String.format(Locale.ROOT, "%.6g", multiplier));
            status = "screen.zhimpact.applied";
        } catch (NumberFormatException ex) {
            status = "screen.zhimpact.invalid";
        }
    }

    private void cyclePreset() {
        double[] presets = {0.0D, 0.25D, 1.0D, 2.0D, 10.0D, 100.0D,
                10000.0D, 1000000.0D, 1000000000.0D,
                -0.25D, -1.0D, -10.0D, -100.0D, -1000000.0D, -1000000000.0D};
        int next = 0;
        for (int i = 0; i < presets.length; i++) {
            if (Double.compare(presets[i], multiplier) == 0) {
                next = (i + 1) % presets.length;
                break;
            }
        }
        multiplier = presets[next];
        ImpactConfig.MULTIPLIER.set(multiplier);
        multiplierInput.setText(String.format(Locale.ROOT, "%.6g", multiplier));
        status = "screen.zhimpact.preset_applied";
    }

    private TranslationTextComponent enabledText() {
        return new TranslationTextComponent("screen.zhimpact.toggle",
                new TranslationTextComponent(ImpactConfig.ENABLED.get()
                        ? "screen.zhimpact.on" : "screen.zhimpact.off"));
    }

    @Override
    public void render(MatrixStack matrixStack, int mouseX, int mouseY, float partialTicks) {
        this.renderBackground(matrixStack);
        drawCenteredString(matrixStack, this.font, this.title, this.width / 2,
                this.height / 2 - 112, 0xFFFFFF);
        drawCenteredString(matrixStack, this.font, "Forge 1.16.5 | U",
                this.width / 2, this.height / 2 - 98, 0xA0A0A0);
        drawCenteredString(matrixStack, this.font,
                new TranslationTextComponent("screen.zhimpact.range"),
                this.width / 2, this.height / 2 + 65, 0xA0A0A0);
        drawCenteredString(matrixStack, this.font, new TranslationTextComponent(status),
                this.width / 2, this.height / 2 + 78, 0x55FF55);
        super.render(matrixStack, mouseX, mouseY, partialTicks);
    }

    @Override
    public boolean isPauseScreen() { return false; }
}
