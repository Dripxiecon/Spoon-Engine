package com.spoonengine.demo;

import android.app.Activity;
import android.graphics.Color;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public final class GlProbeActivity extends Activity {
    private GLSurfaceView surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        CrashDiagnostics.setStage("OpenGL-only test: creating surface");
        getWindow().setStatusBarColor(Color.rgb(12, 19, 28));
        getWindow().setNavigationBarColor(Color.rgb(12, 19, 28));

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.rgb(12, 19, 28));

        surface = new GLSurfaceView(this);
        surface.setEGLContextClientVersion(2);
        surface.setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                CrashDiagnostics.setStage("OpenGL ES 2.0 context created");
                GLES20.glClearColor(0.10f, 0.22f, 0.30f, 1.0f);
            }

            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {
                GLES20.glViewport(0, 0, width, height);
            }

            @Override
            public void onDrawFrame(GL10 gl) {
                GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            }
        });
        surface.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        root.addView(surface, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        TextView label = new TextView(this);
        label.setText("OPENGL ES 2.0 TEST");
        label.setTextColor(Color.WHITE);
        label.setTextSize(16);
        label.setGravity(Gravity.CENTER);
        label.setPadding(dp(16), dp(10), dp(16), dp(10));
        FrameLayout.LayoutParams labelParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        labelParams.topMargin = dp(18);
        root.addView(label, labelParams);

        Button back = new Button(this);
        back.setText("BACK");
        back.setOnClickListener(v -> finish());
        FrameLayout.LayoutParams backParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        backParams.bottomMargin = dp(20);
        root.addView(back, backParams);

        setContentView(root);
        CrashDiagnostics.setStage("OpenGL-only test screen running");
    }

    @Override
    protected void onPause() {
        if (surface != null) surface.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (surface != null) surface.onResume();
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
