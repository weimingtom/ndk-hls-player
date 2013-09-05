package test.hls.ffmpeg;

import android.util.Log;

public class CallBackBridge {
	private static final String logTagNDK = "CallBackBridge";

	public static void PrintNdkLog(String slog) {
		Log.e(logTagNDK, slog);
		return;
	}
}
