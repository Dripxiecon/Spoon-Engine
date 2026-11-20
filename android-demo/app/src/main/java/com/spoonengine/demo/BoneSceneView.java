package com.spoonengine.demo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

public final class BoneSceneView extends View {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final float[] state = new float[9];
    private float yaw = -0.48f;
    private float pitch = 0.28f;
    private float zoom = 1.0f;
    private float lastX;
    private float lastY;
    private boolean dragging;
    private final ScaleGestureDetector scaleDetector;

    public BoneSceneView(Context context) {
        super(context);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        scaleDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override public boolean onScale(ScaleGestureDetector detector) {
                zoom = clamp(zoom * detector.getScaleFactor(), 0.65f, 2.2f);
                invalidate();
                return true;
            }
        });
        setContentDescription("Interactive three-dimensional muscle and bone simulation. Drag to orbit, pinch to zoom.");
    }

    public void setState(float[] values) {
        if (values == null || values.length < state.length) return;
        System.arraycopy(values, 0, state, 0, state.length);
        invalidate();
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        final float w = getWidth();
        final float h = getHeight();
        if (w <= 0 || h <= 0) return;

        canvas.drawColor(Color.rgb(12, 19, 28));
        drawGrid(canvas, w, h);

        Vec a = new Vec(state[0], state[1], state[2]);
        Vec b = new Vec(state[3], state[4], state[5]);
        Vec mid = scale(add(a, b), 0.5f);
        float activation = clamp(state[6], 0f, 1f);
        float muscleWidth = 0.075f + activation * 0.075f;

        Vec aOuter = add(a, new Vec(-0.48f, 0f, 0f));
        Vec bOuter = add(b, new Vec(0.48f, 0f, 0f));

        drawCylinder(canvas, aOuter, a, 0.105f, 0.085f, Color.rgb(174, 194, 211));
        drawCylinder(canvas, b, bOuter, 0.085f, 0.105f, Color.rgb(174, 194, 211));
        drawCylinder(canvas, a, b, muscleWidth, muscleWidth * 0.82f, Color.rgb(255, 133, 92));
        drawJoint(canvas, a, 0.145f, Color.rgb(195, 221, 238));
        drawJoint(canvas, b, 0.145f, Color.rgb(195, 221, 238));

        drawHud(canvas, w, h, activation, state[7], state[8]);
        drawOrbitHint(canvas, w, h);
    }

    private void drawGrid(Canvas canvas, float w, float h) {
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1f);
        paint.setColor(0x263F6078);
        for (int i = -5; i <= 5; i++) {
            line3(canvas, new Vec(i * 0.35f, -0.55f, -1.1f), new Vec(i * 0.35f, -0.55f, 1.1f), w, h);
            line3(canvas, new Vec(-1.75f, -0.55f, i * 0.35f), new Vec(1.75f, -0.55f, i * 0.35f), w, h);
        }
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawCylinder(Canvas canvas, Vec from, Vec to, float radiusA, float radiusB, int color) {
        Screen p1 = project(from);
        Screen p2 = project(to);
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float length = (float)Math.sqrt(dx * dx + dy * dy);
        if (length < 1f) return;
        float nx = -dy / length;
        float ny = dx / length;
        float depthA = p1.scale * radiusA * getWidth() * 0.52f;
        float depthB = p2.scale * radiusB * getWidth() * 0.52f;

        Path body = new Path();
        body.moveTo(p1.x + nx * depthA, p1.y + ny * depthA);
        body.quadTo((p1.x + p2.x) * 0.5f + nx * Math.max(depthA, depthB) * 1.05f,
                (p1.y + p2.y) * 0.5f + ny * Math.max(depthA, depthB) * 1.05f,
                p2.x + nx * depthB, p2.y + ny * depthB);
        body.lineTo(p2.x - nx * depthB, p2.y - ny * depthB);
        body.quadTo((p1.x + p2.x) * 0.5f - nx * Math.max(depthA, depthB) * 1.05f,
                (p1.y + p2.y) * 0.5f - ny * Math.max(depthA, depthB) * 1.05f,
                p1.x - nx * depthA, p1.y - ny * depthA);
        body.close();

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(color);
        canvas.drawPath(body, paint);
        paint.setColor(0x88FFFFFF);
        paint.setStrokeWidth(Math.max(1.5f, depthA * 0.12f));
        canvas.drawLine(p1.x + nx * depthA * 0.35f, p1.y + ny * depthA * 0.35f,
                p2.x + nx * depthB * 0.35f, p2.y + ny * depthB * 0.35f, paint);
        paint.setColor(Color.BLACK);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.4f);
        canvas.drawPath(body, paint);
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawJoint(Canvas canvas, Vec point, float radius, int color) {
        Screen p = project(point);
        float r = Math.max(5f, p.scale * radius * getWidth() * 0.52f);
        paint.setColor(0x55000000);
        canvas.drawCircle(p.x + r * 0.15f, p.y + r * 0.25f, r * 1.08f, paint);
        paint.setColor(color);
        canvas.drawCircle(p.x, p.y, r, paint);
        paint.setColor(0xAAFFFFFF);
        canvas.drawCircle(p.x - r * 0.28f, p.y - r * 0.3f, r * 0.24f, paint);
        paint.setColor(0xFF293C4C);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.5f);
        canvas.drawCircle(p.x, p.y, r, paint);
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawHud(Canvas canvas, float w, float h, float activation, float tension, float distance) {
        paint.setColor(0xFFEAF2F8);
        paint.setTextSize(15f * getResources().getDisplayMetrics().scaledDensity);
        paint.setFakeBoldText(true);
        canvas.drawText("LIVE MUSCLE RIG", dp(16), dp(27), paint);
        paint.setFakeBoldText(false);
        paint.setColor(0xFF9BB0C1);
        paint.setTextSize(11f * getResources().getDisplayMetrics().scaledDensity);
        canvas.drawText("AGONIST  /  TWO BONE BODIES", dp(16), dp(46), paint);

        float barLeft = dp(16);
        float barRight = w - dp(16);
        float barTop = h - dp(58);
        paint.setColor(0xFF263747);
        canvas.drawRoundRect(new RectF(barLeft, barTop, barRight, barTop + dp(7)), dp(4), dp(4), paint);
        paint.setColor(0xFFB7F36B);
        canvas.drawRoundRect(new RectF(barLeft, barTop, barLeft + (barRight - barLeft) * activation, barTop + dp(7)), dp(4), dp(4), paint);
        paint.setColor(0xFFEAF2F8);
        paint.setTextSize(11f * getResources().getDisplayMetrics().scaledDensity);
        canvas.drawText(String.format(java.util.Locale.US, "ACTIVATION %.0f%%     TENSION %.1f N", activation * 100f, tension), barLeft, barTop - dp(10), paint);
        paint.setColor(0xFF9BB0C1);
        canvas.drawText(String.format(java.util.Locale.US, "ATTACHMENT GAP %.3f m", distance), barLeft, h - dp(25), paint);
    }

    private void drawOrbitHint(Canvas canvas, float w, float h) {
        paint.setColor(0x668EA2B4);
        paint.setTextSize(10f * getResources().getDisplayMetrics().scaledDensity);
        canvas.drawText("DRAG TO ORBIT  ·  PINCH TO ZOOM", dp(16), h - dp(8), paint);
    }

    private void line3(Canvas canvas, Vec a, Vec b, float w, float h) {
        Screen p1 = project(a);
        Screen p2 = project(b);
        paint.setColor(0x263F6078);
        paint.setStrokeWidth(1f);
        canvas.drawLine(p1.x, p1.y, p2.x, p2.y, paint);
    }

    private Screen project(Vec point) {
        float cy = (float)Math.cos(yaw), sy = (float)Math.sin(yaw);
        float cp = (float)Math.cos(pitch), sp = (float)Math.sin(pitch);
        float x = point.x * cy - point.z * sy;
        float z = point.x * sy + point.z * cy;
        float y = point.y * cp - z * sp;
        z = point.y * sp + z * cp;
        float camera = 3.7f / zoom;
        float perspective = camera / Math.max(1.0f, camera - z);
        float scale = Math.min(getWidth(), getHeight()) * 0.43f * perspective;
        return new Screen(getWidth() * 0.5f + x * scale,
                getHeight() * 0.52f - y * scale, perspective);
    }

    @Override public boolean onTouchEvent(MotionEvent event) {
        scaleDetector.onTouchEvent(event);
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                lastX = event.getX();
                lastY = event.getY();
                dragging = true;
                return true;
            case MotionEvent.ACTION_POINTER_DOWN:
                dragging = false;
                return true;
            case MotionEvent.ACTION_MOVE:
                if (dragging && event.getPointerCount() == 1 && !scaleDetector.isInProgress()) {
                    float dx = event.getX() - lastX;
                    float dy = event.getY() - lastY;
                    yaw += dx * 0.008f;
                    pitch = clamp(pitch + dy * 0.006f, -1.1f, 1.1f);
                    lastX = event.getX();
                    lastY = event.getY();
                    invalidate();
                }
                return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                dragging = false;
                performClick();
                return true;
            default:
                return true;
        }
    }

    @Override public boolean performClick() {
        super.performClick();
        return true;
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static float clamp(float v, float lo, float hi) {
        return Math.max(lo, Math.min(hi, v));
    }
    private static Vec add(Vec a, Vec b) { return new Vec(a.x+b.x, a.y+b.y, a.z+b.z); }
    private static Vec scale(Vec a, float s) { return new Vec(a.x*s, a.y*s, a.z*s); }

    private static final class Vec {
        final float x, y, z;
        Vec(float x, float y, float z) { this.x=x; this.y=y; this.z=z; }
    }
    private static final class Screen {
        final float x, y, scale;
        Screen(float x, float y, float scale) { this.x=x; this.y=y; this.scale=scale; }
    }
}
