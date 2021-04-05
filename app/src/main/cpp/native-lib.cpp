#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/log.h>
#include <opencv2/core/ocl.hpp>


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

    __android_log_print(ANDROID_LOG_DEBUG, "save directory", "%s", s);

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

void MosaicImage(Mat& img_mosaic)
{
    //__android_log_print(ANDROID_LOG_DEBUG, "ASDF", "INIT");

    Mat img_temp;

    Size originSize = Size(img_mosaic.rows, img_mosaic.cols);

    resize(img_mosaic, img_temp, Size(9, 9));

    resize(img_temp, img_mosaic, originSize);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opencvcameraexample_MainActivity_detect(JNIEnv *env, jobject thiz,
                                                         jlong cascade_classifier_face,
                                                         jlong cascade_classifier_side_face,
                                                         jlong mat_addr_input,
                                                         jlong mat_addr_result, jdoubleArray ROIarray) {

    // not copy
    jdouble *ptrROIarray = env -> GetDoubleArrayElements(ROIarray, JNI_FALSE);

    int size = env->GetArrayLength(ROIarray);

    int realArrayIndex = size / 5;


    // frame 1씩 더해줌
    for(int i = 0; i < 10; i++)
    {
        ptrROIarray[ 5 * i + 4] += 1;
    }

    Mat &img_input = *(Mat *) mat_addr_input;
    Mat &img_result = *(Mat *) mat_addr_result;

    img_result = img_input.clone();

    std::vector<Rect> faces;
    std::vector<Rect> side_faces;

    Mat img_gray;

    cvtColor(img_input, img_gray, COLOR_BGR2GRAY);

    equalizeHist(img_gray, img_gray);

    Mat img_resize;

    float resizeRatio = resize(img_gray, img_resize, 495);


    //-- Detect faces

    ((CascadeClassifier *) cascade_classifier_face)->detectMultiScale( img_resize, faces, 1.1, 5, 0, Size(20, 20) );
    ((CascadeClassifier *) cascade_classifier_side_face)->detectMultiScale( img_resize, side_faces, 1.1, 5, 0, Size(20, 20) );

    __android_log_print(ANDROID_LOG_DEBUG, (char *) "native-lib :: ",

                        (char *) "face %d found ", faces.size());

    // 배열 안에 근처의 사각형 있다 없다 나타내는 flag
    bool rectFlag = false;

    // 삽입 위치 찾았다?
    bool findFlag = false;

    // 삽입 위치 지정
    int insertLocation = -1;

    //frontalface
    for (int i = 0; i < faces.size(); i++) {

        double w_temp = faces[i].width / resizeRatio;

        double real_facesize_x = faces[i].x / resizeRatio + 0.25 * w_temp;

        double h_temp = faces[i].height / resizeRatio;

        double real_facesize_y = faces[i].y / resizeRatio + h_temp * 0.25;

        double real_facesize_width = w_temp * 0.5;

        double real_facesize_height = h_temp * 0.5;

        Rect face_area(real_facesize_x, real_facesize_y, real_facesize_width, real_facesize_height);

        //rectangle(img_result, face_area, Scalar(0,0,255), 5, LINE_8, 0);

        // shallow copy
        Mat img_mosaic = img_result(face_area);

        MosaicImage(img_mosaic);

        for(int preFrameFaceIndex = 0; preFrameFaceIndex < realArrayIndex; preFrameFaceIndex++){
            // width == 0?
            if(ptrROIarray[5 * preFrameFaceIndex + 2] == 0)
            {
                if(findFlag == false)
                {
                    insertLocation = preFrameFaceIndex;
                    findFlag = true;
                }
            }
            // 근처에 있는지 판별 해야함
            else{
               // x + 1/4width < this.x < x + 3/4width && y + 1/4height < this.y < y + 3 / 4 height 이면 같은 rect이다
               if(ptrROIarray[5 * preFrameFaceIndex] < real_facesize_x + real_facesize_width
                  && ptrROIarray[5 * preFrameFaceIndex] + ptrROIarray[5 * preFrameFaceIndex + 2] > real_facesize_x
                  && ptrROIarray[5 * preFrameFaceIndex + 1] < real_facesize_y + real_facesize_height
                  && ptrROIarray[5 * preFrameFaceIndex + 1] + ptrROIarray[5 * preFrameFaceIndex + 3] > real_facesize_y){
                   ptrROIarray[5 * preFrameFaceIndex] = real_facesize_x;
                   ptrROIarray[5 * preFrameFaceIndex + 1] = real_facesize_y;
                   ptrROIarray[5 * preFrameFaceIndex + 2] = real_facesize_width;
                   ptrROIarray[5 * preFrameFaceIndex + 3] = real_facesize_height;
                   ptrROIarray[5 * preFrameFaceIndex + 4] = 0;

                   rectFlag = true;

                   break;
               }
            }
        }

        // empty element를 찾고, 같은 rect를 찾지 못하였을때
        if(findFlag == true && insertLocation > -1 && rectFlag == false)
        {
            // insetLocation자리에넣는다.
            ptrROIarray[5 * insertLocation] = real_facesize_x;
            ptrROIarray[5 * insertLocation + 1] = real_facesize_y;
            ptrROIarray[5 * insertLocation + 2] = real_facesize_width;
            ptrROIarray[5 * insertLocation + 3] = real_facesize_height;

            // frame = 0;
            ptrROIarray[5 * insertLocation + 4] = 0;

            findFlag = false;
            insertLocation = -1;
        }
    }


    //side face
    for (int i = 0; i < side_faces.size(); i++) {

        double w_temp = side_faces[i].width / resizeRatio;

        double real_facesize_x = side_faces[i].x / resizeRatio + 0.25 * w_temp;

        double h_temp = side_faces[i].height / resizeRatio;

        double real_facesize_y = side_faces[i].y / resizeRatio + 0.25 * h_temp;

        double real_facesize_width = w_temp * 0.5;

        double real_facesize_height = h_temp * 0.5;

        Rect face_area(real_facesize_x, real_facesize_y, real_facesize_width,real_facesize_height);

        // shallow copy
        Mat img_mosaic = img_result(face_area);

        MosaicImage(img_mosaic);

        for(int preFrameFaceIndex = 0; preFrameFaceIndex < realArrayIndex; preFrameFaceIndex++){
            // width == 0?
            if(ptrROIarray[5 * preFrameFaceIndex + 2] == 0)
            {
                if(findFlag == false)
                {
                    insertLocation = preFrameFaceIndex;
                    findFlag = true;
                }
            }
                // 근처에 있는지 판별 해야함
                // if (RectA.Left < RectB.Right && RectA.Right > RectB.Left && RectA.Top > RectB.Bottom && RectA.Bottom < RectB.Top )
            else{
                // 겹치면 같은 rect
                if(ptrROIarray[5 * preFrameFaceIndex] < real_facesize_x + real_facesize_width
                        && ptrROIarray[5 * preFrameFaceIndex] + ptrROIarray[5 * preFrameFaceIndex + 2] > real_facesize_x
                        && ptrROIarray[5 * preFrameFaceIndex + 1] < real_facesize_y + real_facesize_height
                        && ptrROIarray[5 * preFrameFaceIndex + 1] + ptrROIarray[5 * preFrameFaceIndex + 3] > real_facesize_y){

                    ptrROIarray[5 * preFrameFaceIndex] = real_facesize_x;
                    ptrROIarray[5 * preFrameFaceIndex + 1] = real_facesize_y;
                    ptrROIarray[5 * preFrameFaceIndex + 2] = real_facesize_width;
                    ptrROIarray[5 * preFrameFaceIndex + 3] = real_facesize_height;
                    ptrROIarray[5 * preFrameFaceIndex + 4] = 0;

                    rectFlag = true;

                    break;
                }
            }
        }

        // empty element를 찾고, 같은 rect를 찾지 못하였을때
        if(findFlag == true && insertLocation > -1 && rectFlag == false)
        {
            // insetLocation자리에넣는다.
            ptrROIarray[5 * insertLocation] = real_facesize_x;
            ptrROIarray[5 * insertLocation + 1] = real_facesize_y;
            ptrROIarray[5 * insertLocation + 2] = real_facesize_width;
            ptrROIarray[5 * insertLocation + 3] = real_facesize_height;

            //frame = 0;
            ptrROIarray[5 * insertLocation + 4] = 0;

            findFlag = false;
            insertLocation = -1;
        }
    }

    // pre Frame face mosaic
    for (int i = 0; i < realArrayIndex; i++)
    {
        // width !=0 && frame != 0
        if( ptrROIarray[2 + 5 * i] != 0 && ptrROIarray[4 + 5 * i] != 0)
        {
            // 프레임이 어느정도 넘으면 (어느 프레임 이상 기존의 rect 불러왔으면)
            if(ptrROIarray[4 + 5 * i] > 15)
            {
                // 초기화
                ptrROIarray[5 * i] = 0;
                ptrROIarray[5 * i + 1] = 0;
                ptrROIarray[5 * i + 2] = 0;
                ptrROIarray[5 * i + 3] = 0;
                ptrROIarray[5 * i + 4] = 0;

                continue;
            }
            Rect face_area( ptrROIarray[5 * i],ptrROIarray[5 * i + 1],ptrROIarray[5 * i + 2],ptrROIarray[5 * i + 3] );

            rectangle(img_result, face_area, Scalar(255, 0, 0), 1, LINE_8, 0);

            Mat img_mosaic = img_result(face_area);

            MosaicImage(img_mosaic);
        }
    }

    env -> ReleaseDoubleArrayElements(ROIarray, ptrROIarray, 0);
}

