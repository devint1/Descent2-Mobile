package tuchsen.descent2;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Surface;
import android.view.View;
import android.view.WindowManager;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Locale;

/**
 * Created by devin on 4/17/16.
 */
public class DescentActivity extends Activity implements SensorEventListener {
	private DescentView descentView;
	private MediaPlayer mediaPlayer;
	private Sensor gyroscopeSensor;
	private SensorManager sensorManager;
	private float buttonSizeBias;
	private float acceleration[];
	private int mediaPlayerPosition;
	private int refreshPeriodUs;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		DisplayMetrics metrics;
		Resources resources;

		super.onCreate(savedInstanceState);

		// Enable immersive mode and make sure it's enabled whenever we're fullscreen
		setImmersive();
		if (Build.VERSION.SDK_INT >= 19) {
			getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(
					new View.OnSystemUiVisibilityChangeListener() {
						@Override
						public void onSystemUiVisibilityChange(int visibility) {
							if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
								setImmersive();
							}
						}
					});
		}

		// Calculate button size bias; want slightly bigger buttons on bigger screens
		resources = getResources();
		metrics = resources.getDisplayMetrics();
		buttonSizeBias = (float) Math.min(Math.max((metrics.widthPixels / metrics.xdpi
				+ metrics.heightPixels / metrics.ydpi) / 5.5f, 1), 1.4);

		// Set up gyroscope
		sensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
		gyroscopeSensor = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);

		// If we're on SDK 11, absolute values are supported for gyro refresh period. Use the screen's refresh rate to
		// determine refresh period.
		if (Build.VERSION.SDK_INT >= 11) {
			final Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
			final float refreshRate = display.getRefreshRate();
			refreshPeriodUs = (int) (1.0 / refreshRate * 1000000);
		}

		// Create media player for MIDI
		mediaPlayer = new MediaPlayer();

		// Create OpenGL ES view
		descentView = new DescentView(this);

		// Set views
		setContentView(descentView);

		// Keep the screen from going to sleep
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}

	@Override
	protected void onPause() {
		super.onPause();
		descentPause();
		mediaPlayer.pause();
		mediaPlayerPosition = mediaPlayer.getCurrentPosition();
		stopMotion();
	}

	@Override
	protected void onResume() {
		super.onResume();
		setImmersive();
		if (!descentView.getSurfaceWasDestroyed()) {
			descentView.resumeRenderThread();
		}
		mediaPlayer.seekTo(mediaPlayerPosition);
		mediaPlayer.start();
		if (getUseGyroscope()) {
			startMotion();
		}
	}

	@Override
	public void onSensorChanged(SensorEvent event) {
		acceleration = event.values;
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {

	}

	@SuppressWarnings("unused")
	private float[] getRotationRate() {
		if (haveGyroscope()) {
			if (getWindowManager().getDefaultDisplay().getRotation() == Surface.ROTATION_90) {
				acceleration[0] *= -1;
				acceleration[1] *= -1;
			}
			return acceleration;
		} else {
			return new float[]{0, 0, 0};
		}
	}

	private boolean haveGyroscope() {
		return gyroscopeSensor != null;
	}

	private void startMotion() {
		// According to this, API 11 can specify the delay as an absolute value
		// Some devices have high refresh rate displays, so a lower delay is needed to improve gyro responsiveness
		// https://developer.android.com/guide/topics/sensors/sensors_overview
		if (Build.VERSION.SDK_INT >= 11) {
			sensorManager.registerListener(this, gyroscopeSensor, refreshPeriodUs);
		} else {
			sensorManager.registerListener(this, gyroscopeSensor, SensorManager.SENSOR_DELAY_GAME);
		}
	}

	private void stopMotion() {
		sensorManager.unregisterListener(this);
	}

	@SuppressWarnings("unused")
	private void playMidi(String path, boolean looping) {
		File file = new File(path);
		FileDescriptor fd;
		FileInputStream fos;

		try {
			fos = new FileInputStream(file);
			fd = fos.getFD();
			mediaPlayer.setDataSource(fd);
			mediaPlayer.prepare();
		} catch (IOException e) {
			e.printStackTrace();
		}
		mediaPlayer.setLooping(looping);
		mediaPlayer.start();
	}

	@SuppressWarnings("unused")
	private boolean playRedbookTrack(int tracknum, boolean looping) {
		AssetFileDescriptor fd = null;
		AssetManager assetManager = getAssets();
		String trackList[];

		try {
			trackList = assetManager.list("music");
			for (String track : trackList) {
				if (track.matches(String.format(Locale.getDefault(), "%02d.*", tracknum))) {
					fd = assetManager.openFd("music/" + track);
					break;
				}
			}
			if (fd != null) {
				mediaPlayer.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
				mediaPlayer.prepare();
			} else {
				return false;
			}
		} catch (IOException e) {
			return false;
		}
		mediaPlayer.setLooping(looping);
		mediaPlayer.start();
		return true;
	}

	@SuppressWarnings("unused")
	private int getRedbookTrackCount() {
		AssetManager assetManager = getAssets();
		String trackList[];

		try {
			trackList = assetManager.list("music");
			return Integer.parseInt(trackList[trackList.length - 1].substring(0, 2));
		} catch (Exception e) {
			return 0;
		}
	}

	@SuppressWarnings("unused")
	private void stopMusic() {
		mediaPlayer.stop();
		mediaPlayer.reset();
	}

	@SuppressWarnings("unused")
	private void setMusicVolume(float volume) {
		mediaPlayer.setVolume(volume, volume);
	}

	@SuppressWarnings("unused")
	private float dpToPx(float dp) {
		Resources resources = getResources();
		DisplayMetrics metrics = resources.getDisplayMetrics();
		return dp * (((float) metrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT) * buttonSizeBias);
	}

	@SuppressWarnings("unused")
	private float pxToDp(float px) {
		Resources resources = getResources();
		DisplayMetrics metrics = resources.getDisplayMetrics();
		return px / (((float) metrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT) * buttonSizeBias);
	}

	/**
	 * Enables immersive mode, hiding navigation controls
	 */
	private void setImmersive() {
		if (Build.VERSION.SDK_INT >= 19) {
			getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_STABLE
							| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
							| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
							| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
							| View.SYSTEM_UI_FLAG_FULLSCREEN
							| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
		}
	}

	private static native void descentPause();

	private static native boolean getUseGyroscope();

	static {
		System.loadLibrary("descent2");
	}
}
