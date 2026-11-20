package com.spoonengine.demo;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static {
        System.loadLibrary("spoon_demo");
    }

    private TextView readout;
    private TextView activationLabel;
    private int activation = 75;

    private static native String nativeReset();
    private static native String nativeStep(float activation, int frames);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(22), dp(28), dp(22), dp(22));
        root.setBackgroundColor(0xFF10151D);

        TextView title = new TextView(this);
        title.setText("SPOON / PHYSICS LAB");
        title.setTextColor(0xFFB7F36B);
        title.setTextSize(23);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("MUSCLE → FORCE → BONE MOTION");
        subtitle.setTextColor(0xFF9AA8B8);
        subtitle.setTextSize(12);
        subtitle.setPadding(0, dp(6), 0, dp(22));
        root.addView(subtitle);

        activationLabel = new TextView(this);
        activationLabel.setTextColor(0xFFFFFFFF);
        activationLabel.setTextSize(16);
        root.addView(activationLabel);

        SeekBar strength = new SeekBar(this);
        strength.setMax(100);
        strength.setProgress(activation);
        root.addView(strength, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(48)));
        strength.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int value, boolean fromUser) {
                activation = value;
                updateLabel();
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {
                runSimulation(240);
            }
        });

        Button step = new Button(this);
        step.setText("RUN 2 SECONDS");
        root.addView(step);
        step.setOnClickListener(view -> runSimulation(240));

        Button reset = new Button(this);
        reset.setText("RESET RIG");
        root.addView(reset);
        reset.setOnClickListener(view -> {
            activation = 75;
            strength.setProgress(activation);
            readout.setText(nativeReset());
            updateLabel();
        });

        readout = new TextView(this);
        readout.setTextColor(0xFFE7EDF5);
        readout.setTextSize(15);
        readout.setTypeface(Typeface.MONOSPACE);
        readout.setLineSpacing(dp(5), 1.0f);
        readout.setPadding(dp(16), dp(18), dp(16), dp(18));
        readout.setBackgroundColor(0xFF1B2532);
        LinearLayout.LayoutParams readoutParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        readoutParams.topMargin = dp(18);
        root.addView(readout, readoutParams);

        TextView footer = new TextView(this);
        footer.setText("NATIVE C++17  •  ARM64  •  FIXED 120 HZ");
        footer.setTextColor(0xFF718096);
        footer.setTextSize(10);
        footer.setGravity(Gravity.CENTER);
        footer.setPadding(0, dp(18), 0, 0);
        root.addView(footer);

        setContentView(root);
        readout.setText(nativeReset());
        updateLabel();
    }

    private void runSimulation(int frames) {
        readout.setText(nativeStep(activation / 100.0f, frames));
    }

    private void updateLabel() {
        activationLabel.setText("MUSCLE POWER     " + activation + "%");
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
