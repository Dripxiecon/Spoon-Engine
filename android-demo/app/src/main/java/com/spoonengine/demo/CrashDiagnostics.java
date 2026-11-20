package com.spoonengine.demo;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.view.ViewGroup;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

public final class CrashDiagnostics {
    private static final String REPORT_FILE = "spoon-crash-report.txt";
    private static volatile String stage = "Application process started";

    private CrashDiagnostics() {}

    public static void setStage(String value) {
        stage = value == null ? "Unknown stage" : value;
    }

    public static synchronized void record(Context context, String reason, Throwable error) {
        try {
            StringBuilder report = new StringBuilder();
            report.append("SPOON ENGINE CRASH REPORT\n");
            report.append("Stage: ").append(stage).append('\n');
            report.append("Reason: ").append(reason).append('\n');
            report.append("Device: ").append(android.os.Build.MANUFACTURER).append(' ')
                    .append(android.os.Build.MODEL).append('\n');
            report.append("Android: ").append(android.os.Build.VERSION.RELEASE)
                    .append(" (API ").append(android.os.Build.VERSION.SDK_INT).append(")\n\n");
            if (error != null) {
                java.io.StringWriter trace = new java.io.StringWriter();
                error.printStackTrace(new java.io.PrintWriter(trace));
                report.append(trace);
            } else {
                report.append("No Java exception details were available. The process may have been terminated natively.");
            }
            File file = new File(context.getFilesDir(), REPORT_FILE);
            try (FileOutputStream output = new FileOutputStream(file, false)) {
                output.write(report.toString().getBytes(StandardCharsets.UTF_8));
                output.flush();
                output.getFD().sync();
            }
        } catch (Throwable ignored) {
            // Diagnostics must never cause another application crash.
        }
    }

    public static synchronized String readReport(Context context) {
        File file = new File(context.getFilesDir(), REPORT_FILE);
        if (!file.isFile()) return "No saved crash report yet.";
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int count;
            while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
            return output.toString("UTF-8");
        } catch (Throwable error) {
            return "Could not read crash report: " + error;
        }
    }

    public static void showPrevious(Activity activity) {
        if (activity == null || activity.isFinishing()) return;
        File file = new File(activity.getFilesDir(), REPORT_FILE);
        if (!file.isFile()) return;
        String report = readReport(activity);
        ScrollView scroll = new ScrollView(activity);
        TextView text = new TextView(activity);
        text.setText(report);
        text.setTextSize(12);
        text.setTypeface(Typeface.MONOSPACE);
        int padding = Math.round(14 * activity.getResources().getDisplayMetrics().density);
        text.setPadding(padding, padding, padding, padding);
        scroll.addView(text, new ScrollView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        new AlertDialog.Builder(activity)
                .setTitle("Previous crash report")
                .setMessage("Spoon saved details from the last crash. Share this report so the fault can be identified.")
                .setView(scroll)
                .setPositiveButton("SHARE REPORT", (dialog, which) -> share(activity, report))
                .setNegativeButton("CLOSE", null)
                .setNeutralButton("DELETE REPORT", (dialog, which) -> {
                    if (file.exists()) file.delete();
                })
                .show();
    }

    private static void share(Context context, String report) {
        try {
            Intent intent = new Intent(Intent.ACTION_SEND);
            intent.setType("text/plain");
            intent.putExtra(Intent.EXTRA_SUBJECT, "Spoon Engine crash report");
            intent.putExtra(Intent.EXTRA_TEXT, report);
            context.startActivity(Intent.createChooser(intent, "Share crash report"));
        } catch (Throwable error) {
            record(context, "Could not open report sharing", error);
        }
    }
}
