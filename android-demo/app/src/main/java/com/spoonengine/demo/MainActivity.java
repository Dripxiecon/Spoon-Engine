package com.spoonengine.demo;

import android.app.Activity;
import android.content.Intent;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Choreographer;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public final class MainActivity extends Activity implements Choreographer.FrameCallback {
    private boolean nativeLoaded;

    private BoneGLView scene;
    private TextView status;
    private int activation = 75;
    private boolean running;
    private boolean framePosted;
    private long previousFrameNanos;
    private double accumulator;
    private String lastReadout = "";

    private static native String nativeReset();
    private static native String nativeStep(float activation, int frames);
    private static native float[] nativeSceneState();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.rgb(12, 19, 28));

        scene = new BoneGLView(this);
        root.addView(scene, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout topBar = new LinearLayout(this);
        topBar.setOrientation(LinearLayout.HORIZONTAL);
        topBar.setGravity(Gravity.CENTER_VERTICAL);
        topBar.setPadding(dp(12), dp(8), dp(12), dp(8));
        topBar.setBackgroundColor(0xCC101923);

        Button menu = new Button(this);
        menu.setText("≡");
        menu.setTextSize(25);
        menu.setTextColor(0xFFB7F36B);
        menu.setAllCaps(false);
        menu.setMinWidth(dp(48));
        menu.setMinimumWidth(dp(48));
        menu.setPadding(0, 0, 0, 0);
        topBar.addView(menu, new LinearLayout.LayoutParams(dp(48), dp(44)));

        TextView heading = new TextView(this);
        heading.setText("SPOON  /  PHYSICS LAB");
        heading.setTextColor(0xFFEAF2F8);
        heading.setTextSize(13);
        heading.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        heading.setPadding(dp(10), 0, 0, 0);
        topBar.addView(heading, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        FrameLayout.LayoutParams topParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.TOP);
        root.addView(topBar, topParams);

        LinearLayout bottomBar = new LinearLayout(this);
        bottomBar.setOrientation(LinearLayout.HORIZONTAL);
        bottomBar.setGravity(Gravity.CENTER_VERTICAL);
        bottomBar.setPadding(dp(12), dp(8), dp(12), dp(12));
        bottomBar.setBackgroundColor(0xCC101923);

        status = new TextView(this);
        status.setText("LIVE  ·  120 Hz FIXED STEP");
        status.setTextColor(0xFFB7F36B);
        status.setTextSize(11);
        status.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        bottomBar.addView(status, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        Button pause = new Button(this);
        pause.setText("Ⅱ  PAUSE");
        pause.setTextSize(12);
        pause.setAllCaps(false);
        pause.setTextColor(0xFF101923);
        pause.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xFFB7F36B));
        bottomBar.addView(pause, new LinearLayout.LayoutParams(dp(116), dp(44)));

        FrameLayout.LayoutParams bottomParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        root.addView(bottomBar, bottomParams);

        setContentView(root);
        CrashDiagnostics.setStage("Loading native physics library");
        try {
            System.loadLibrary("spoon_demo");
            nativeLoaded = true;
            CrashDiagnostics.setStage("Creating initial physics scene");
            lastReadout = nativeReset();
            refreshScene();
            CrashDiagnostics.setStage("Application running");
        } catch (Throwable error) {
            nativeLoaded = false;
            CrashDiagnostics.record(this, "Startup initialization failed", error);
            status.setText("STARTUP ERROR  ·  OPEN MENU FOR REPORT");
        }
        CrashDiagnostics.showPrevious(this);

        menu.setOnClickListener(view -> showControls());
        heading.setOnClickListener(view -> CrashDiagnostics.showPrevious(this));
        pause.setOnClickListener(view -> {
            running = !running;
            accumulator = 0.0;
            previousFrameNanos = 0L;
            pause.setText(running ? "Ⅱ  PAUSE" : "▶  RESUME");
            status.setText(running ? "LIVE  ·  120 Hz FIXED STEP" : "PAUSED  ·  SIMULATION FROZEN");
            if (running) startFrames();
        });
    }

    private void showControls() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(20), dp(12), dp(20), dp(4));

        TextView activationTitle = new TextView(this);
        activationTitle.setText("MUSCLE ACTIVATION  ·  " + activation + "%");
        activationTitle.setTextSize(15);
        activationTitle.setTextColor(0xFF17212B);
        activationTitle.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        panel.addView(activationTitle);

        SeekBar strength = new SeekBar(this);
        strength.setMax(100);
        strength.setProgress(activation);
        panel.addView(strength, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(48)));
        strength.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                activation = value;
                activationTitle.setText("MUSCLE ACTIVATION  ·  " + activation + "%");
            }
            @Override public void onStartTrackingTouch(SeekBar bar) {}
            @Override public void onStopTrackingTouch(SeekBar bar) {}
        });

        TextView details = new TextView(this);
        details.setText(lastReadout);
        details.setTextColor(0xFF344454);
        details.setTextSize(12);
        details.setTypeface(Typeface.MONOSPACE);
        details.setPadding(0, dp(12), 0, dp(8));
        panel.addView(details);

        Button reset = new Button(this);
        reset.setText("RESET SIMULATION");
        reset.setAllCaps(false);
        panel.addView(reset);
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("SIMULATION CONTROLS")
                .setView(panel)
                .setPositiveButton("CLOSE", null)
                .create();
        reset.setOnClickListener(view -> {
            if (!nativeLoaded) {
                details.setText(CrashDiagnostics.readReport(this));
                return;
            }
            try {
                lastReadout = nativeReset();
                accumulator = 0.0;
                refreshScene();
                details.setText(lastReadout);
            } catch (Throwable error) {
                CrashDiagnostics.record(this, "Physics reset failed", error);
                details.setText(CrashDiagnostics.readReport(this));
            }
        });
        dialog.show();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (scene != null) scene.onResume();
        running = true;
        startFrames();
    }

    @Override
    protected void onPause() {
        running = false;
        stopFrames();
        if (scene != null) scene.onPause();
        super.onPause();
    }

    private void startFrames() {
        if (running && !framePosted) {
            previousFrameNanos = 0L;
            Choreographer.getInstance().postFrameCallback(this);
            framePosted = true;
        }
    }

    private void stopFrames() {
        if (framePosted) {
            Choreographer.getInstance().removeFrameCallback(this);
            framePosted = false;
        }
        previousFrameNanos = 0L;
        accumulator = 0.0;
    }

    @Override
    public void doFrame(long frameTimeNanos) {
        framePosted = false;
        if (!running || !nativeLoaded) return;

        if (previousFrameNanos != 0L) {
            double frameDelta = Math.min(
                    (frameTimeNanos - previousFrameNanos) / 1_000_000_000.0, 0.1);
            accumulator += frameDelta;
            final double fixedDelta = 1.0 / 120.0;
            int steps = Math.min(8, (int)(accumulator / fixedDelta));
            if (steps > 0) {
                try {
                    lastReadout = nativeStep(activation / 100.0f, steps);
                } catch (Throwable error) {
                    running = false;
                    CrashDiagnostics.record(this, "Physics update failed", error);
                    if (status != null) status.setText("PHYSICS ERROR  ·  REPORT SAVED");
                    return;
                }
                accumulator -= steps * fixedDelta;
                if (steps == 8 && accumulator >= fixedDelta) {
                    accumulator %= fixedDelta;
                }
                if (status != null && !lastReadout.isEmpty()) {
                    status.setText("LIVE  ·  " + activation + "% MUSCLE  ·  120 Hz");
                }
            }
        }
        previousFrameNanos = frameTimeNanos;
        refreshScene();
        if (running && !framePosted) {
            Choreographer.getInstance().postFrameCallback(this);
            framePosted = true;
        }
    }

    private void refreshScene() {
        if (scene != null && nativeLoaded) {
            try {
                scene.setState(nativeSceneState());
            } catch (Throwable error) {
                running = false;
                CrashDiagnostics.record(this, "Reading physics scene failed", error);
                if (status != null) status.setText("SCENE ERROR  ·  REPORT SAVED");
            }
        }
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
