package com.spoonengine.demo;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private TextView reportView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        CrashDiagnostics.setStage("Safe-mode diagnostic screen");
        getWindow().setStatusBarColor(Color.rgb(12, 19, 28));
        getWindow().setNavigationBarColor(Color.rgb(12, 19, 28));

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(20), dp(28), dp(20), dp(20));
        root.setBackgroundColor(Color.rgb(12, 19, 28));

        TextView title = new TextView(this);
        title.setText("SPOON  /  SAFE MODE");
        title.setTextColor(0xFFB7F36B);
        title.setTextSize(21);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("Physics and OpenGL are disabled in this build. This screen checks whether the Android app itself can start.");
        subtitle.setTextColor(0xFFEAF2F8);
        subtitle.setTextSize(14);
        subtitle.setPadding(0, dp(14), 0, dp(18));
        root.addView(subtitle);

        TextView status = new TextView(this);
        status.setText("SAFE MODE RUNNING");
        status.setTextColor(0xFFB7F36B);
        status.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        status.setTextSize(13);
        root.addView(status);

        Button refresh = new Button(this);
        refresh.setText("REFRESH CRASH REPORT");
        refresh.setAllCaps(false);
        root.addView(refresh, fullWidth());
        
        ScrollView scroll = new ScrollView(this);
        reportView = new TextView(this);
        reportView.setTextColor(0xFFEAF2F8);
        reportView.setTextSize(12);
        reportView.setTypeface(Typeface.MONOSPACE);
        reportView.setPadding(dp(12), dp(12), dp(12), dp(12));
        scroll.addView(reportView);
        LinearLayout.LayoutParams scrollParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        scrollParams.topMargin = dp(12);
        root.addView(scroll, scrollParams);

        Button share = new Button(this);
        share.setText("SHARE REPORT");
        share.setAllCaps(false);
        root.addView(share, fullWidth());

        setContentView(root);
        refresh.setOnClickListener(v -> refreshReport());
        share.setOnClickListener(v -> shareReport());
        refreshReport();
        CrashDiagnostics.setStage("Safe-mode diagnostic screen running");
    }

    private void refreshReport() {
        String report = CrashDiagnostics.readReport(this);
        reportView.setText(report);
    }

    private void shareReport() {
        String report = CrashDiagnostics.readReport(this);
        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_SUBJECT, "Spoon Engine crash report");
        intent.putExtra(Intent.EXTRA_TEXT, report);
        startActivity(Intent.createChooser(intent, "Share crash report"));
    }

    private LinearLayout.LayoutParams fullWidth() {
        return new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
