package com.example.opencvcameraexample;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.annotation.TargetApi;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.imgproc.Imgproc;
import org.opencv.video.Video;
import org.opencv.videoio.VideoCapture;
import org.opencv.videoio.VideoWriter;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.concurrent.Semaphore;

import static android.Manifest.permission.CAMERA;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.os.Environment.DIRECTORY_PICTURES;
import static java.lang.Thread.getAllStackTraces;
import static java.lang.Thread.sleep;


public class MainActivity extends AppCompatActivity
        implements CameraBridgeViewBase.CvCameraViewListener2 {

    private int cameraIndex = 0;

    private static final String TAG = "opencv";
    private Mat matInput;
    private Mat matResult;

    // 10명까지 저장
    private double[] ROIarray = new double[50];

    private boolean isVideo;
    private boolean isVideoCapturing;

    //
    private String tempVideoPath;

    private CameraBridgeViewBase mOpenCvCameraView;

    private VideoWriter videoWriter;

    //public native void ConvertRGBtoGray(long matAddrInput, long matAddrResult);
    public native long loadCascade(String cascadeFileName);
    public native void detect(long cascadeClassifier_face,
                              long cascadeClassifier_side_face, long matAddrInput, long matAddrResult, double[] array);
    public long cascadeClassifier_face = 0;
    public long cascadeClassifier_side_face = 0;

    private final Semaphore writeLock = new Semaphore(1);

    public void getWriteLock() throws InterruptedException {
        writeLock.acquire();
    }

    public void releaseWriteLock() {
        writeLock.release();
    }



    static {
        System.loadLibrary("opencv_java4");
        System.loadLibrary("native-lib");
    }

    private void copyFile(String filename) {

        //String baseDir = Environment.getExternalStorageDirectory().getPath();
        String baseDir = getExternalFilesDir(null).toString();
        String pathDir = baseDir + File.separator + filename;

        AssetManager assetManager = this.getAssets();

        InputStream inputStream = null;
        OutputStream outputStream = null;

        try {
            inputStream = assetManager.open(filename);
            outputStream = new FileOutputStream(pathDir);

            byte[] buffer = new byte[1024];
            int read;

            while ((read = inputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, read);
            }

            inputStream.close();
            inputStream = null;
            outputStream.flush();
            outputStream.close();
            outputStream = null;
        } catch (Exception e) {
            Log.d(TAG, "copyFile :: 파일 복사 중 예외 발생 "+e.toString() );
        }

    }

    private void read_cascade_file(){
        copyFile("haarcascade_frontalface_default.xml");
        copyFile("haarcascade_profileface.xml");

        Log.d(TAG, "read_cascade_file:");

        cascadeClassifier_face = loadCascade( "haarcascade_frontalface_default.xml");
        Log.d(TAG, "read_cascade_file:");

        cascadeClassifier_side_face = loadCascade( "haarcascade_profileface.xml");
    }




    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_main);

        mOpenCvCameraView = (CameraBridgeViewBase)findViewById(R.id.activity_surface_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setCameraIndex(cameraIndex); // front-camera(1),  back-camera(0)

        videoWriter = new VideoWriter("", VideoWriter.fourcc('M','J','P','G'), 25.0D, new Size(mOpenCvCameraView.getWidth(), mOpenCvCameraView.getHeight()));

        isVideo = false;
        isVideoCapturing = false;

        Button captureButton = (Button)findViewById(R.id.capture_button);

        captureButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                File path = new File(Environment.getExternalStoragePublicDirectory(DIRECTORY_PICTURES) + "/PublicCamera/");
                path.mkdirs();

                long now = System.currentTimeMillis();
                Date mDate= new Date(now);
                SimpleDateFormat simpleDate = new SimpleDateFormat("yyyy_MM_dd_hh_mm_ss_SSS");
                String getTime = simpleDate.format(mDate);
                // picture
                if(isVideo == false) {
                    try {
                        mOpenCvCameraView.disableView();

                        getWriteLock();

                        getTime += ".jpg";
                        File file = new File(path, getTime);

                        String filename = file.toString();

                        Imgproc.cvtColor(matResult, matResult, Imgproc.COLOR_BGR2RGBA);

                        boolean ret  = Imgcodecs.imwrite( filename, matResult);
                        if ( ret ) {
                            Log.d(TAG, "SUCESS");
                            Toast.makeText(getApplicationContext(), "캡쳐 완료", Toast.LENGTH_SHORT).show();
                        }
                        else {
                            Log.d(TAG, "FAIL");
                            Toast.makeText(getApplicationContext(), "캡쳐 실패", Toast.LENGTH_SHORT).show();
                        }


                        Intent mediaScanIntent = new Intent( Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                        mediaScanIntent.setData(Uri.fromFile(file));
                        sendBroadcast(mediaScanIntent);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    releaseWriteLock();

                    try {
                        sleep(100);
                    }
                    catch (InterruptedException e)
                    {
                        e.printStackTrace();
                    }
                    mOpenCvCameraView.enableView();
                }
                // video
                else{
                    Log.d("aaaaaaaa", "video");

                    getTime += ".avi";
                    File file = new File(path, getTime);

                    String filename = file.toString();

                    tempVideoPath = filename;
                    videoWriter.open(filename, VideoWriter.fourcc('M','J','P','G'), 25.0D, new Size(mOpenCvCameraView.getWidth(), mOpenCvCameraView.getHeight()));
                }

            }
        });

        Button changeCameraButton = (Button)findViewById(R.id.change_button);
        changeCameraButton.setOnClickListener(new View.OnClickListener(){
            public void onClick(View v){
                try{
                    if(cameraIndex == 0) {
                        mOpenCvCameraView.setVisibility(SurfaceView.INVISIBLE);
                        cameraIndex = 1;

                        mOpenCvCameraView.setCameraIndex(cameraIndex);
                        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
                    }
                    else if (cameraIndex == 1) {
                        mOpenCvCameraView.setVisibility(SurfaceView.INVISIBLE);
                        cameraIndex = 0;

                        mOpenCvCameraView.setCameraIndex(cameraIndex);
                        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
                    }
                    else {
                    }
                }catch (Exception e)
                {
                    Log.d("cameraIndex", e.getMessage());
                }

            }
        });


        Button changeCaptureButton = (Button)findViewById(R.id.change_capture_button);
        changeCaptureButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                if(isVideo == true){
                    isVideo = false;
                    changeCaptureButton.setText("비디오");
                    captureButton.setText("캡쳐");
                } else{
                    isVideo = true;
                    changeCaptureButton.setText("사진");
                    captureButton.setText("녹화");
                }
            }
        });
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
        {
            mOpenCvCameraView.setBackgroundColor(Color.BLACK);

        }
    }

    @Override
    public void onBackPressed()
    {
        mOpenCvCameraView.setBackgroundColor(Color.BLACK);

        try{
            sleep(100);
            super.onBackPressed();
        }
        catch (InterruptedException e) {
            e.printStackTrace();
        }

    }

    @Override
    public void onResume() {
        mOpenCvCameraView.setBackgroundColor(Color.TRANSPARENT);
        super.onResume();

        if (!OpenCVLoader.initDebug()) {
            Log.d(TAG, "onResume :: Internal OpenCV library not found.");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_2_0, this, mLoaderCallback);
        } else {
            Log.d(TAG, "onResume :: OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }


    public void onDestroy() {
        super.onDestroy();

        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onCameraViewStarted(int width, int height) {

    }

    @Override
    public void onCameraViewStopped() {

    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {

        if(isVideo == false)
        {
            try {
                getWriteLock();
                matInput = inputFrame.rgba();
                if (matResult == null)
                    matResult = new Mat(matInput.rows(), matInput.cols(), matInput.type());
                //ConvertRGBtoGray(matInput.getNativeObjAddr(), matResult.getNativeObjAddr());
                if(cameraIndex == 1)
                    Core.flip(matInput, matInput, 1); // 가로

                detect(cascadeClassifier_face, cascadeClassifier_side_face, matInput.getNativeObjAddr(), matResult.getNativeObjAddr(), ROIarray);
            } catch (InterruptedException e){
                e.printStackTrace();
            }

            releaseWriteLock();
        } else {
            try {
                getWriteLock();
                matInput = inputFrame.rgba();
                if (matResult == null) {
                    matResult = new Mat(matInput.rows(), matInput.cols(), matInput.type());
                }
                else {
                }

                //ConvertRGBtoGray(matInput.getNativeObjAddr(), matResult.getNativeObjAddr());
                if(cameraIndex == 1)
                    Core.flip(matInput, matInput, 1); // 가로

                detect(cascadeClassifier_face, cascadeClassifier_side_face, matInput.getNativeObjAddr(), matResult.getNativeObjAddr(), ROIarray);

                //Log.d("aaaaaaaaaaaa", "" + matResult.width() + "     " + inputFrame.rgba().width());
                //videoWriter.write(matResult);
            } catch (InterruptedException e)
            {
                Log.d("aaaaa", "          e            ");
                e.printStackTrace();
            }
            releaseWriteLock();
        }


        return matResult;
    }


    protected List<? extends CameraBridgeViewBase> getCameraViewList() {
        return Collections.singletonList(mOpenCvCameraView);
    }


    //여기서부턴 퍼미션 관련 메소드
    private static final int CAMERA_PERMISSION_REQUEST_CODE = 200;


    protected void onCameraPermissionGranted() {
        List<? extends CameraBridgeViewBase> cameraViews = getCameraViewList();
        if (cameraViews == null) {
            return;
        }
        for (CameraBridgeViewBase cameraBridgeViewBase: cameraViews) {
            if (cameraBridgeViewBase != null) {
                cameraBridgeViewBase.setCameraPermissionGranted();

                read_cascade_file();
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        boolean havePermission = true;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(CAMERA) != PackageManager.PERMISSION_GRANTED
            || checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{CAMERA, WRITE_EXTERNAL_STORAGE}, CAMERA_PERMISSION_REQUEST_CODE);
                havePermission = false;
            }
        }
        if (havePermission) {
            onCameraPermissionGranted();
        }
    }

    @Override
    @TargetApi(Build.VERSION_CODES.M)
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode == CAMERA_PERMISSION_REQUEST_CODE && grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED && grantResults[1] == PackageManager.PERMISSION_GRANTED) {
            onCameraPermissionGranted();
        }else{
            showDialogForPermission("앱을 실행하려면 퍼미션을 허가하셔야합니다.");
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }


    @TargetApi(Build.VERSION_CODES.M)
    private void showDialogForPermission(String msg) {

        AlertDialog.Builder builder = new AlertDialog.Builder( MainActivity.this);
        builder.setTitle("알림");
        builder.setMessage(msg);
        builder.setCancelable(false);
        builder.setPositiveButton("예", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id){
                requestPermissions(new String[]{CAMERA, WRITE_EXTERNAL_STORAGE}, CAMERA_PERMISSION_REQUEST_CODE);
            }
        });
        builder.setNegativeButton("아니오", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface arg0, int arg1) {
                finish();
            }
        });
        builder.create().show();
    }


}