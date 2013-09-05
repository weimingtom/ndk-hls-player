package test.hls.ffmpeg;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.app.Activity;
import android.graphics.PixelFormat;

public class MainActivity extends Activity implements Callback {

	protected static final String TAG = "MainActivity";
	Thread mythread = null;

		
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		System.loadLibrary("hls");
		PrintNdkLog("tt");
		
		((SurfaceView) findViewById(R.id.surface1)).getHolder().addCallback(
				this);
		// convert();
		
	}

	public void PrintNdkLog(String slog) {
		Log.e("---", slog);
		return;
	}

	public native int convert();

	public native void test();

	public native void destorySurface();

	public native void setSurface(Surface s);

	public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {

		setSurface(arg0.getSurface());
	}

	public void surfaceCreated(SurfaceHolder arg0) {
		setSurface(arg0.getSurface());
		arg0.setFormat(PixelFormat.RGBA_8888);
		Log.d("", "w1:" + arg0.getSurfaceFrame().width());

		
		
		synchronized (this) {

			if (mythread == null) {
				mythread = new Thread(new Runnable() {

					public void run() {
						long startTime=System.currentTimeMillis();
						test();
						Log.d(TAG,"total process time:"+(System.currentTimeMillis()-startTime));

						
					}
				});
				
				mythread.start();
			}

		}


		
		Log.d("", "w2:" + arg0.getSurfaceFrame().width());

	}

	public void surfaceDestroyed(SurfaceHolder arg0) {
		destorySurface();
	}

	public void runInMaintThread(Runnable run) {
		new Handler(this.getMainLooper()).post(run);
	}
}
