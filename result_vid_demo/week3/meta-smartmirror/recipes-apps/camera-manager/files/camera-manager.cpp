#include <opencv2/opencv.hpp>
#include <iostream>
#include <atomic>

using namespace cv;
using namespace std;

enum ViewMode { DUAL, SINGLE };

struct MirrorState {
    atomic<ViewMode> mode;
    atomic<int> currentCam; // 0 = front, 1 = rear
};

void switchMode(MirrorState& s) {
    s.mode = (s.mode == DUAL ? SINGLE : DUAL);
    cout << "Mode switched to: " << (s.mode == DUAL ? "DUAL" : "SINGLE") << endl;
}

void switchCamera(MirrorState& s) {
    s.currentCam = (s.currentCam == 0 ? 1 : 0);
    cout << "Camera switched to: " << (s.currentCam == 0 ? "Front (USB)" : "Rear (IMX708)") << endl;
}

int main() {
    VideoCapture cam_front(0); // USB OV3660
    VideoCapture cam_rear(1);  // RPi Camera v3 IMX708

    if (!cam_front.isOpened() || !cam_rear.isOpened()) {
        cerr << "Cannot open one or both cameras!" << endl;
        return -1;
    }

    // Optimize latency
    cam_front.set(CAP_PROP_BUFFERSIZE, 1);
    cam_rear.set(CAP_PROP_BUFFERSIZE, 1);
    cam_front.set(CAP_PROP_FRAME_WIDTH, 640);
    cam_front.set(CAP_PROP_FRAME_HEIGHT, 480);
    cam_rear.set(CAP_PROP_FRAME_WIDTH, 640);
    cam_rear.set(CAP_PROP_FRAME_HEIGHT, 480);
    cam_front.set(CAP_PROP_FPS, 30);
    cam_rear.set(CAP_PROP_FPS, 30);

    namedWindow("SmartMirror", WINDOW_NORMAL);
    setWindowProperty("SmartMirror", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    MirrorState state{DUAL, 0};

    setMouseCallback("SmartMirror", [](int event, int x, int y, int, void* userdata) {
        if (event == EVENT_LBUTTONDOWN) {
            auto* s = reinterpret_cast<MirrorState*>(userdata);
            if (s->mode == DUAL) {
                switchMode(*s); // 2 cam → 1 cam
            } else {
                if (x < 640)
                    switchCamera(*s); // switch camera
                else
                    switchMode(*s); // return to 2 cam mode
            }
        }
    }, &state);

    cout << "SmartMirror started! Touch to toggle mode/camera." << endl;

    Mat f, r, display;
    while (true) {
        cam_front >> f;
        cam_rear >> r;
        if (f.empty() || r.empty()) continue;

        if (state.mode == DUAL) {
            resize(f, f, Size(640, 480));
            resize(r, r, Size(640, 480));
            hconcat(f, r, display);
        } else {
            if (state.currentCam == 0)
                resize(f, display, Size(1280, 720));
            else
                resize(r, display, Size(1280, 720));
        }

        imshow("SmartMirror", display);
        if (waitKey(1) == 27) break; // ESC for exit (debug))
    }

    return 0;
}
