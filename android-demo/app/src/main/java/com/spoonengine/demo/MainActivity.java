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
    private BoneSceneView scene;
    private int activation = 75;

    private static native String nativeReset();
    private static native String nativeStep(float activation, int frames);
    private static native float[] nativeSceneState();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(18), dp(22), dp(18), dp(16));
        root.setBackgroundColor(0xFF10151D);

        TextView title = new TextView(this);
        title.setText("SPOON / PHYSICS LAB");
        title.setTextColor(0xFFB7F36B);
        title.setTextSize(22);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("BONE + MUSCLE  /  INTERACTIVE 3D VIEW");
        subtitle.setTextColor(0xFF9AA8B8);
        subtitle.setTextSize(11);
        subtitle.setPadding(0, dp(5), 0, dp(12));
        root.addView(subtitle);

        scene = new BoneSceneView(this);
        LinearLayout.LayoutParams sceneParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f);
        sceneParams.bottomMargin = dp(12);
        root.addView(scene, sceneParams);

        activationLabel = new TextView(this);
        activationLabel.setTextColor(0xFFFFFFFF);
        activationLabel.setTextSize(14);
        root.addView(activationLabel);

        SeekBar strength = new SeekBar(this);
        strength.setMax(100);
        strength.setProgress(activation);
        root.addView(strength, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(42)));
        strength.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int value, boolean fromUser) {
                activation = value;
                updateLabel();
                if (fromUser) {
                    nativeStep(activation / 100.0f, 1);
                    refreshScene();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        LinearLayout actions = new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        Button step = new Button(this);
        step.setText("RUN 2 SECONDS");
        actions.addView(step, new LinearLayout.LayoutParams(0, dp(50), 1f));
        Button reset = new Button(this);
        reset.setText("RESET");
        actions.addView(reset, new LinearLayout.LayoutParams(0, dp(50), 1f));
        root.addView(actions);

        readout = new TextView(this);
        readout.setTextColor(0xFFE7EDF5);
        readout.setTextSize(12);
        readout.setTypeface(Typeface.MONOSPACE);
        readout.setLineSpacing(dp(3), 1.0f);
        readout.setPadding(dp(12), dp(10), dp(12), dp(10));
        readout.setBackgroundColor(0xFF1B2532);
        LinearLayout.LayoutParams readoutParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        readoutParams.topMargin = dp(8);
        root.addView(readout, readoutParams);

        TextView footer = new TextView(this);
        footer.setText("C++17  •  ARM64  •  DRAG TO ORBIT  •  PINCH TO ZOOM");
        footer.setTextColor(0xFF718096);
        footer.setTextSize(9);
        footer.setGravity(Gravity.CENTER);
        footer.setPadding(0, dp(8), 0, 0);
        root.addView(footer);

        setContentView(root);
        readout.setText(nativeReset());
        updateLabel();
        refreshScene();

        step.setOnClickListener(view -> runSimulation(240));
        reset.setOnClickListener(view -> {
            activation = 75;
            strength.setProgress(activation);
            readout.setText(nativeReset());
            updateLabel();
            refreshScene();
        });
    }

    private void runSimulation(int frames) {
        readout.setText(nativeStep(activation / 100.0f, frames));
        refreshScene();
    }

    private void refreshScene() {
        if (scene != null) scene.setState(nativeSceneState());
    }

    private void updateLabel() {
        activationLabel.setText("MUSCLE ACTIVATION     " + activation + "%");
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
