package com.spoonengine.demo;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

public final class NativeProbeActivity extends Activity {
    private TextView resultView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        CrashDiagnostics.setStage("Native library probe screen");
        getWindow().setStatusBarColor(Color.rgb(12, 19, 28));
        getWindow().setNavigationBarColor(Color.rgb(12, 19, 28));

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(dp(24), dp(24), dp(24), dp(24));
        root.setBackgroundColor(Color.rgb(12, 19, 28));

        TextView title = new TextView(this);
        title.setText("SPOON / NATIVE TEST");
        title.setTextColor(0xFFB7F36B);
        title.setTextSize(21);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        root.addView(title);

        TextView explanation = new TextView(this);
        explanation.setText("This test only loads Spoon's C++ library. It does not create physics objects or start the simulation.");
        explanation.setTextColor(Color.WHITE);
        explanation.setTextSize(14);
        explanation.setGravity(Gravity.CENTER);
        explanation.setPadding(0, dp(16), 0, dp(20));
        root.addView(explanation);

        Button test = new Button(this);
        test.setText("LOAD NATIVE LIBRARY");
        test.setAllCaps(false);
        root.addView(test);
        test.setOnClickListener(v -> loadLibrary(test));

        resultView = new TextView(this);
        resultView.setText("WAITING — tap the button to test.");
        resultView.setTextColor(0xFFEAF2F8);
        resultView.setTextSize(14);
        resultView.setGravity(Gravity.CENTER);
        resultView.setPadding(0, dp(20), 0, dp(20));
        root.addView(resultView);

        Button back = new Button(this);
        back.setText("BACK");
        back.setAllCaps(false);
        root.addView(back);
        back.setOnClickListener(v -> finish());

        setContentView(root);
    }

    private void loadLibrary(Button test) {
        test.setEnabled(false);
        CrashDiagnostics.setStage("Native probe: loading spoon_demo shared library");
        try {
            System.loadLibrary("spoon_demo");
            CrashDiagnostics.setStage("Native probe: spoon_demo library loaded");
            resultView.setText("SUCCESS\n\nThe C++ library loaded. No physics objects have been created.");
            resultView.setTextColor(0xFFB7F36B);
        } catch (Throwable error) {
            CrashDiagnostics.setStage("Native probe: library load failed");
            resultView.setText("LOAD FAILED\n\n" + error.getClass().getSimpleName()
                    + ": " + String.valueOf(error.getMessage()));
            resultView.setTextColor(0xFFFF8A80);
            test.setEnabled(true);
        }
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
