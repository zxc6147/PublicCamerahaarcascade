#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/log.h>


using namespace cv;
using namespace std;

/*
extern "C"
JNIEXPORT void JNICALL
Java_com_example_opencvcameraexample_MainActivity_ConvertRGBtoGray(JNIEnv *env, jobject thiz,
                                                                   jlong mat_addr_input,
                                                                   jlong mat_addr_result) {
    Mat &matInput = *(Mat *)mat_addr_input;
    Mat &matResult = *(Mat *)mat_addr_result;

    cvtColor(matInput, matResult, COLOR_RGBA2GRAY);
}
*/

float resize(Mat img_src, Mat &img_resize, int resize_width){


    float scale = resize_width / (float)img_src.cols ;

    if (img_src.cols > resize_width) {

        int new_height = cvRound(img_src.rows * scale);

        resize(img_src, img_resize, Size(resize_width, new_height));

    }

    else {

        img_resize = img_src;

    }
    return scale;
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_opencvcameraexample_MainActivity_loadCascade(JNIEnv *env, jobject thiz,
                                                              jstring cascade_file_name) {

    const char *nativeFileNameString = env->GetStringUTFChars(cascade_file_name, 0);


    string baseDir("/storage/emulated/0/Android/data/com.example.opencvcameraexample/files/");

    baseDir.append(nativeFileNameString);

    // log
    std::vector<char> writable(baseDir.begin(), baseDir.end());
    writable.push_back('\0');
    char* s = &writable[0];

    __android_log_print(ANDROID_LOG_DEBUG, "AAAA", "%s", s);

    const char *pathDir = baseDir.c_str();
    // end_log

    jlong ret = 0;

    ret = (jlong) new CascadeClassifier(baseDir);

    if (((CascadeClassifier *) ret)->empty()) {

        __android_log_print(ANDROID_LOG_DEBUG, "native-lib :: ",

                            "CascadeClassifier로 로딩 실패  %s", nativeFileNameString);

    }

    else

        __android_log_print(ANDROID_LOG_DEBUG, "native-lib :: ",

                            "CascadeClassifier로 로딩 성공 %s", nativeFileNameString);



    env->ReleaseStringUTFChars(cascade_file_name, nativeFileNameString);


    return ret;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opencvcameraexample_MainActivity_detect(JNIEnv *env, jobject thiz,
                                                         jlong cascade_classifier_face,
                                                         jlong cascade_classifier_eye,
                                                         jlong mat_addr_input,
                                                         jlong mat_addr_result) {
    Mat &img_input = *(Mat *) mat_addr_input;

    Mat &img_result = *(Mat *) mat_addr_result;


    img_result = img_input.clone();

    std::vector<Rect> faces;

    Mat img_gray;

    cvtColor(img_input, img_gray, COLOR_BGR2GRAY);

    equalizeHist(img_gray, img_gray);


    Mat img_resize;

    float resizeRatio = resize(img_gray, img_resize, 495);


    //-- Detect faces

    ((CascadeClassifier *) cascade_classifier_face)->detectMultiScale( img_resize, faces, 1.1, 3, 0, Size(20, 20) );

    __android_log_print(ANDROID_LOG_DEBUG, (char *) "native-lib :: ",

                        (char *) "face %d found ", faces.size());


    for (int i = 0; i < faces.size(); i++) {

        double real_facesize_x = faces[i].x / resizeRatio;

        double real_facesize_y = faces[i].y / resizeRatio;

        double real_facesize_width = faces[i].width / resizeRatio;

        double real_facesize_height = faces[i].height / resizeRatio;

        Rect face_area(real_facesize_x, real_facesize_y, real_facesize_width,real_facesize_height);

        // shallow copy
        Mat img_mosaic = img_result(face_area);

        // temp를 mosaic shallow copy하면역시 동작 x
        // 다른 객체여야 동작한다
        Mat img_temp;

        Size originSize = Size(img_mosaic.rows, img_mosaic.cols);


        /*

        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix -2:: ",

                            (char *) "%p pointer", img_mosaic.ptr());

        resize(img_mosaic, img_temp, Size(20, 20));

        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix -1:: ",

                            (char *) "%p pointer", img_mosaic.ptr());

        resize(img_temp, img_mosaic, originSize);

        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix 0 :: ",

                            (char *) "%p pointer", img_mosaic.ptr());

*/

        // ok


        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix -2:: ",

                            (char *) "%p pointer", img_mosaic.ptr());
        // why not working?

        resize(img_mosaic, img_mosaic, Size(20, 20),0 ,0 ,INTER_LINEAR);

        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix -1:: ",

                            (char *) "%p pointer", img_mosaic.ptr());


        resize(img_mosaic, img_mosaic, originSize,0 ,0 ,INTER_LINEAR);

        __android_log_print(ANDROID_LOG_DEBUG, (char *) "resized matrix 0 :: ",

                            (char *) "%p pointer", img_mosaic.ptr());

        // blur
        rectangle(img_result, face_area, Scalar(0, 0, 255), 2, LINE_8, 0);
    }
}
