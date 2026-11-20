package com.spoonengine.demo;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.view.MotionEvent;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.Locale;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public final class BoneGLView extends GLSurfaceView {
    private final SceneRenderer renderer;
    private float lastX;
    private float lastY;
    private float lastSpan;
    private boolean multiTouch;

    public BoneGLView(Context context) {
        super(context);
        setEGLContextClientVersion(2);
        setPreserveEGLContextOnPause(true);
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        renderer = new SceneRenderer();
        setRenderer(renderer);
        setContentDescription("Real 3D bone and muscle simulation. Drag to orbit, pinch to zoom.");
    }

    public void setState(float[] values) {
        renderer.setState(values);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int count = event.getPointerCount();
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                lastX = event.getX();
                lastY = event.getY();
                multiTouch = false;
                return true;
            case MotionEvent.ACTION_POINTER_DOWN:
                multiTouch = true;
                lastSpan = span(event);
                return true;
            case MotionEvent.ACTION_MOVE:
                if (count >= 2) {
                    float current = span(event);
                    if (lastSpan > 0f && current > 0f) {
                        renderer.zoomBy(current / lastSpan);
                    }
                    lastSpan = current;
                    multiTouch = true;
                } else if (!multiTouch) {
                    float x = event.getX();
                    float y = event.getY();
                    renderer.orbit((x - lastX) * 0.45f, (y - lastY) * 0.35f);
                    lastX = x;
                    lastY = y;
                }
                return true;
            case MotionEvent.ACTION_POINTER_UP:
                lastSpan = 0f;
                multiTouch = true;
                return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                multiTouch = false;
                return true;
            default:
                return true;
        }
    }

    private static float span(MotionEvent event) {
        if (event.getPointerCount() < 2) return 0f;
        float dx = event.getX(0) - event.getX(1);
        float dy = event.getY(0) - event.getY(1);
        return (float)Math.sqrt(dx * dx + dy * dy);
    }

    private static final class SceneRenderer implements GLSurfaceView.Renderer {
        private static final String VERTEX_SHADER =
                "attribute vec3 aPosition; attribute vec3 aNormal;" +
                "uniform mat4 uMvp; uniform mat4 uModel;" +
                "varying float vLight;" +
                "void main(){ vec3 n=normalize(mat3(uModel)*aNormal);" +
                "vec3 l=normalize(vec3(-0.35,0.75,0.58));" +
                "vLight=0.30+0.70*max(dot(n,l),0.0);" +
                "gl_Position=uMvp*vec4(aPosition,1.0); }";
        private static final String FRAGMENT_SHADER =
                "precision mediump float; uniform vec4 uColor; varying float vLight;" +
                "void main(){ gl_FragColor=vec4(uColor.rgb*vLight,uColor.a); }";

        private final float[] state = new float[23];
        private final float[] projection = new float[16];
        private final float[] view = new float[16];
        private final float[] viewProjection = new float[16];
        private final float[] model = new float[16];
        private final float[] mvp = new float[16];
        private FloatBuffer vertices;
        private ShortBuffer indices;
        private int indexCount;
        private int program;
        private int positionHandle;
        private int normalHandle;
        private int mvpHandle;
        private int modelHandle;
        private int colorHandle;
        private float yaw = -18f;
        private float pitch = 12f;
        private float zoom = 3.1f;
        private int width = 1;
        private int height = 1;

        synchronized void setState(float[] values) {
            if (values != null && values.length >= state.length) {
                System.arraycopy(values, 0, state, 0, state.length);
            }
        }

        synchronized void orbit(float dx, float dy) {
            yaw += dx;
            pitch = clamp(pitch + dy, -75f, 75f);
        }

        synchronized void zoomBy(float factor) {
            if (Float.isFinite(factor)) zoom = clamp(zoom / factor, 1.7f, 6.0f);
        }

        @Override
        public void onSurfaceCreated(GL10 unused, EGLConfig config) {
            GLES20.glClearColor(0.047f, 0.075f, 0.11f, 1f);
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);
            GLES20.glEnable(GLES20.GL_CULL_FACE);
            program = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
            positionHandle = GLES20.glGetAttribLocation(program, "aPosition");
            normalHandle = GLES20.glGetAttribLocation(program, "aNormal");
            mvpHandle = GLES20.glGetUniformLocation(program, "uMvp");
            modelHandle = GLES20.glGetUniformLocation(program, "uModel");
            colorHandle = GLES20.glGetUniformLocation(program, "uColor");
            createSphere();
        }

        @Override
        public void onSurfaceChanged(GL10 unused, int w, int h) {
            width = Math.max(1, w);
            height = Math.max(1, h);
            GLES20.glViewport(0, 0, width, height);
            Matrix.perspectiveM(projection, 0, 42f, (float)width / height, 0.08f, 30f);
        }

        @Override
        public void onDrawFrame(GL10 unused) {
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
            if (program == 0 || vertices == null) return;
            float[] s;
            synchronized (this) {
                s = state.clone();
            }
            float yr = (float)Math.toRadians(yaw);
            float pr = (float)Math.toRadians(pitch);
            float eyeX = (float)(Math.sin(yr) * Math.cos(pr) * zoom);
            float eyeY = (float)(Math.sin(pr) * zoom);
            float eyeZ = (float)(Math.cos(yr) * Math.cos(pr) * zoom);
            Matrix.setLookAtM(view, 0, eyeX, eyeY, eyeZ, -0.02f, 0.02f, 0f, 0f, 1f, 0f);
            Matrix.multiplyMM(viewProjection, 0, projection, 0, view, 0);

            drawBone(s, 0, 0.43f, 0.090f, 0.078f, new float[]{0.70f,0.79f,0.86f,1f});
            drawBone(s, 7, 0.43f, 0.078f, 0.070f, new float[]{0.62f,0.73f,0.82f,1f});
            drawSphere(0f, 0f, 0f, 0.105f, 0.105f, 0.105f,
                    1f, 0.78f, 0.42f, 1f);

            float ax=s[14], ay=s[15], az=s[16];
            float bx=s[17], by=s[18], bz=s[19];
            drawMuscle(ax, ay, az, bx, by, bz, s[20]);

            drawSphere(s[0], s[1], s[2], 0.055f, 0.055f, 0.055f,
                    0.84f,0.89f,0.94f,1f);
            drawSphere(s[7], s[8], s[9], 0.052f, 0.052f, 0.052f,
                    0.84f,0.89f,0.94f,1f);
        }

        private void drawBone(float[] s, int offset, float halfLength,
                              float radiusY, float radiusZ, float[] color) {
            drawOrientedEllipsoid(s[offset], s[offset+1], s[offset+2],
                    s[offset+3], s[offset+4], s[offset+5], s[offset+6],
                    halfLength, radiusY, radiusZ, color);
        }

        private void drawMuscle(float ax, float ay, float az,
                                float bx, float by, float bz, float activation) {
            float dx=bx-ax, dy=by-ay, dz=bz-az;
            float length=(float)Math.sqrt(dx*dx+dy*dy+dz*dz);
            if (length < 0.03f) return;
            float inv=1f/length;
            float nx=dx*inv, ny=dy*inv, nz=dz*inv;
            float dot=clamp(nx,-1f,1f);
            float angle=(float)Math.toDegrees(Math.acos(dot));
            float axisX=0f, axisY=-nz, axisZ=ny;
            float axisLength=(float)Math.sqrt(axisY*axisY+axisZ*axisZ);
            float midX=(ax+bx)*0.5f, midY=(ay+by)*0.5f, midZ=(az+bz)*0.5f;
            float bulge=0.070f + clamp(activation,0f,1f)*0.035f;
            Matrix.setIdentityM(model,0);
            Matrix.translateM(model,0,midX,midY,midZ);
            if (axisLength > 0.0001f) {
                Matrix.rotateM(model,0,angle,axisX,axisY/axisLength,axisZ/axisLength);
            } else if (dot < 0f) {
                Matrix.rotateM(model,0,180f,0f,0f,1f);
            }
            Matrix.scaleM(model,0,length*0.57f,bulge,bulge*0.88f);
            drawModel(model, 1f,0.34f,0.22f,1f);
        }

        private void drawOrientedEllipsoid(float x,float y,float z,
                float qw,float qx,float qy,float qz,
                float sx,float sy,float sz,float[] color) {
            Matrix.setIdentityM(model,0);
            Matrix.translateM(model,0,x,y,z);
            float norm=(float)Math.sqrt(qw*qw+qx*qx+qy*qy+qz*qz);
            if (norm > 0.00001f) {
                qw/=norm; qx/=norm; qy/=norm; qz/=norm;
                float[] rotation=new float[16];
                float[] qMatrix=new float[16];
                Matrix.setIdentityM(qMatrix,0);
                float xx=qx*qx, yy=qy*qy, zz=qz*qz;
                float xy=qx*qy, xz=qx*qz, yz=qy*qz;
                float wx=qw*qx, wy=qw*qy, wz=qw*qz;
                qMatrix[0]=1f-2f*(yy+zz); qMatrix[1]=2f*(xy+wz); qMatrix[2]=2f*(xz-wy);
                qMatrix[4]=2f*(xy-wz); qMatrix[5]=1f-2f*(xx+zz); qMatrix[6]=2f*(yz+wx);
                qMatrix[8]=2f*(xz+wy); qMatrix[9]=2f*(yz-wx); qMatrix[10]=1f-2f*(xx+yy);
                Matrix.multiplyMM(rotation,0,model,0,qMatrix,0);
                System.arraycopy(rotation,0,model,0,16);
            }
            Matrix.scaleM(model,0,sx,sy,sz);
            drawModel(model,color[0],color[1],color[2],color[3]);
        }

        private void drawSphere(float x,float y,float z,float sx,float sy,float sz,
                                float r,float g,float b,float a) {
            Matrix.setIdentityM(model,0);
            Matrix.translateM(model,0,x,y,z);
            Matrix.scaleM(model,0,sx,sy,sz);
            drawModel(model,r,g,b,a);
        }

        private void drawModel(float[] transform,float r,float g,float b,float a) {
            Matrix.multiplyMM(mvp,0,viewProjection,0,transform,0);
            GLES20.glUseProgram(program);
            GLES20.glUniformMatrix4fv(mvpHandle,1,false,mvp,0);
            GLES20.glUniformMatrix4fv(modelHandle,1,false,transform,0);
            GLES20.glUniform4f(colorHandle,r,g,b,a);
            vertices.position(0);
            GLES20.glEnableVertexAttribArray(positionHandle);
            GLES20.glVertexAttribPointer(positionHandle,3,GLES20.GL_FLOAT,false,24,vertices);
            vertices.position(3);
            GLES20.glEnableVertexAttribArray(normalHandle);
            GLES20.glVertexAttribPointer(normalHandle,3,GLES20.GL_FLOAT,false,24,vertices);
            GLES20.glDrawElements(GLES20.GL_TRIANGLES,indexCount,GLES20.GL_UNSIGNED_SHORT,indices);
        }

        private void createSphere() {
            final int rings=18, slices=24;
            float[] data=new float[(rings+1)*(slices+1)*6];
            short[] faces=new short[rings*slices*6];
            int v=0;
            for(int y=0;y<=rings;y++) {
                double phi=Math.PI*y/rings;
                float py=(float)Math.cos(phi);
                float ring=(float)Math.sin(phi);
                for(int x=0;x<=slices;x++) {
                    double theta=2.0*Math.PI*x/slices;
                    float px=ring*(float)Math.cos(theta);
                    float pz=ring*(float)Math.sin(theta);
                    data[v++]=px; data[v++]=py; data[v++]=pz;
                    data[v++]=px; data[v++]=py; data[v++]=pz;
                }
            }
            int f=0;
            for(int y=0;y<rings;y++) for(int x=0;x<slices;x++) {
                short a=(short)(y*(slices+1)+x);
                short b=(short)(a+slices+1);
                faces[f++]=a; faces[f++]=b; faces[f++]=(short)(a+1);
                faces[f++]=(short)(a+1); faces[f++]=b; faces[f++]=(short)(b+1);
            }
            vertices=ByteBuffer.allocateDirect(data.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
            vertices.put(data).position(0);
            indices=ByteBuffer.allocateDirect(faces.length*2).order(ByteOrder.nativeOrder()).asShortBuffer();
            indices.put(faces).position(0);
            indexCount=faces.length;
        }

        private static int createProgram(String vertex,String fragment) {
            int vs=compileShader(GLES20.GL_VERTEX_SHADER,vertex);
            int fs=compileShader(GLES20.GL_FRAGMENT_SHADER,fragment);
            int p=GLES20.glCreateProgram();
            GLES20.glAttachShader(p,vs);
            GLES20.glAttachShader(p,fs);
            GLES20.glLinkProgram(p);
            int[] status=new int[1];
            GLES20.glGetProgramiv(p,GLES20.GL_LINK_STATUS,status,0);
            if(status[0]==0) {
                String error=GLES20.glGetProgramInfoLog(p);
                GLES20.glDeleteProgram(p);
                throw new IllegalStateException("OpenGL program link failed: "+error);
            }
            GLES20.glDeleteShader(vs);
            GLES20.glDeleteShader(fs);
            return p;
        }

        private static int compileShader(int type,String source) {
            int shader=GLES20.glCreateShader(type);
            GLES20.glShaderSource(shader,source);
            GLES20.glCompileShader(shader);
            int[] status=new int[1];
            GLES20.glGetShaderiv(shader,GLES20.GL_COMPILE_STATUS,status,0);
            if(status[0]==0) {
                String error=GLES20.glGetShaderInfoLog(shader);
                GLES20.glDeleteShader(shader);
                throw new IllegalStateException("OpenGL shader compile failed: "+error);
            }
            return shader;
        }

        private static float clamp(float v,float min,float max) {
            return Math.max(min,Math.min(max,v));
        }
    }
}
