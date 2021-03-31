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


    __android_log_print(ANDROID_LOG_DEBUG, (char *) "findface :: ",
                        (char *) "Start ");

    img_result = img_input.clone();

    std::vector<Rect> faces;

    Mat img_gray;

    Mat img_resized;


    resize(img_input, img_resized, Size(img_result.cols / 2, img_result.rows / 2), 0, 0,INTER_LINEAR);

    try {
        cvtColor(img_resized, img_gray, COLOR_BGR2GRAY);
        equalizeHist(img_gray, img_gray);
        vector<Rect> faces;
        ((CascadeClassifier *) cascade_classifier_face)->detectMultiScale( img_gray, faces, 1.1, 3, CASCADE_FIND_BIGGEST_OBJECT|CASCADE_SCALE_IMAGE, Size(30, 30) );

        for(int i = 0; i < faces.size(); i++) {
            Mat frame = img_resized;
            Rect face = faces[i];

            int cnts = 0;
            int mb = 9;
            int wPoint = 0;
            int hPoint = 0;
            int xStartPoint = 0;
            int yStartPoint = 0;
            double R = 0;
            double G = 0;
            double B = 0;

            for (int i = 0; i < face.height / mb; i++) {
                for (int j = 0; j < face.width / mb; j++) {
                    cnts = 0;
                    B = 0;
                    G = 0;
                    R = 0;
                    xStartPoint = face.x + (j * mb);
                    yStartPoint = face.y + (i * mb);

                    // 이미지의 픽셀 값의 r, g, b 값의 각각 합을 구함
                    for (int mbY = yStartPoint; mbY < yStartPoint + mb; mbY++) {
                        for (int mbX = xStartPoint; mbX < xStartPoint + mb; mbX++) {
                            wPoint = mbX;
                            hPoint = mbY;

                            if (mbX >= frame.cols) {
                                wPoint = frame.cols - 1;
                            }
                            if (mbY >= frame.rows){
                                hPoint = frame.rows - 1;
                            }

                            cv::Vec3b color = frame.at<cv::Vec3b>(hPoint, wPoint);
                            B += color.val[0];
                            G += color.val[1];
                            R += color.val[2];
                            cnts++;
                        }
                    }

                    // r, g, b 값의 평균 산출
                    B /= cnts;
                    G /= cnts;
                    R /= cnts;

                    // 모자이크 색상 생성
                    cv::Scalar color;
                    color.val[0] = B;
                    color.val[1] = G;
                    color.val[2] = R;

                    // 프레임에 모자이크 이미지 삽입
                    rectangle(
                            frame,
                            Point(xStartPoint, yStartPoint),
                            Point(xStartPoint + mb, yStartPoint + mb),
                            color,
                            -1,
                            8,
                            0
                    );
                }
            }
        }
    }
    catch (Exception& e) {
        __android_log_print(ANDROID_LOG_DEBUG, "detectAndDraw",

                            "인식 혹은 모자이크 실패");
    }



    /*
    cvtColor(img_input, img_gray, COLOR_BGR2GRAY);

    equalizeHist(img_gray, img_gray);


    Mat img_resize;

    float resizeRatio = resize(img_gray, img_resize, 640);






    //-- Detect faces
    ((CascadeClassifier *) cascade_classifier_face)->detectMultiScale( img_resize, faces, 1.1, 3, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );



    __android_log_print(ANDROID_LOG_DEBUG, (char *) "native-lib :: ",

                        (char *) "face %d found ", faces.size());


    for (int i = 0; i < faces.size(); i++) {

        double real_facesize_x = faces[i].x / resizeRatio;

        double real_facesize_y = faces[i].y / resizeRatio;

        double real_facesize_width = faces[i].width / resizeRatio;

        double real_facesize_height = faces[i].height / resizeRatio;


        Point center( real_facesize_x + real_facesize_width / 2, real_facesize_y + real_facesize_height/2);

        /*
        ellipse(img_result, center, Size( real_facesize_width / 2, real_facesize_height / 2), 0, 0, 360,

                Scalar(255, 0, 255), 30, 8, 0);



        Rect face_area(real_facesize_x, real_facesize_y, real_facesize_width,real_facesize_height);

        rectangle(img_result, face_area, Scalar(255, 0, 0), 2, LINE_8, 0);


        Mat faceROI = img_gray( face_area );

        std::vector<Rect> eyes;


        //-- In each face, detect eyes

        ((CascadeClassifier *) cascade_classifier_eye)->detectMultiScale( faceROI, eyes, 1.1, 3, 0 |CASCADE_SCALE_IMAGE, Size(30, 30) );


        for ( size_t j = 0; j < eyes.size(); j++ )

        {

            Point eye_center( real_facesize_x + eyes[j].x + eyes[j].width/2, real_facesize_y + eyes[j].y + eyes[j].height/2 );

            int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );

            circle( img_result, eye_center, radius, Scalar( 255, 0, 0 ), 2, 8, 0 );

        }
*/
    //}



}