package com.spoonengine.demo;

import android.app.Application;

public final class SpoonApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        Thread.setDefaultUncaughtExceptionHandler((thread, error) -> {
            CrashDiagnostics.record(this, "Uncaught exception on thread: " + thread.getName(), error);
            Thread.UncaughtExceptionHandler previous =
                    Thread.getDefaultUncaughtExceptionHandler();
            if (previous != null && previous != Thread.getDefaultUncaughtExceptionHandler()) {
                previous.uncaughtException(thread, error);
            } else {
                android.os.Process.killProcess(android.os.Process.myPid());
                System.exit(10);
            }
        });
    }
}
