#include <opencv2/opencv.hpp>
#include <iostream>
#include <atomic>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>

using namespace cv;
using namespace std;

enum ViewMode { DUAL, SINGLE, AUTO };

struct MirrorState {
    atomic<ViewMode> mode;
    atomic<int> currentCam; // 0 = front, 1 = rear
    atomic<bool> needsRedraw;
    atomic<int> availableCameras; // Số camera thực tế available
};

// Function to check if V4L2 device is a capture device
bool isV4L2CaptureDevice(const string& device_path) {
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd == -1) return false;
    
    struct v4l2_capability caps;
    bool is_capture = false;
    
    if (ioctl(fd, VIDIOC_QUERYCAP, &caps) >= 0) {
        if (caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
            cout << "Found V4L2 capture device: " << device_path 
                 << " (" << caps.card << ")" << endl;
            is_capture = true;
        }
    }
    
    close(fd);
    return is_capture;
}

// Function to detect available V4L2 camera devices
vector<string> detectV4L2Cameras() {
    vector<string> cameras;
    DIR* dir = opendir("/dev");
    struct dirent* entry;
    
    cout << "=== Scanning for V4L2 cameras ===" << endl;
    
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name.find("video") == 0) {
            string path = "/dev/" + name;
            if (isV4L2CaptureDevice(path)) {
                cameras.push_back(path);
            }
        }
    }
    closedir(dir);
    
    // Sort for consistent order
    sort(cameras.begin(), cameras.end());
    
    cout << "Total capture devices found: " << cameras.size() << endl;
    for (size_t i = 0; i < cameras.size(); i++) {
        cout << "  Camera " << i << ": " << cameras[i] << endl;
    }
    
    return cameras;
}

// Function to test if camera can actually be opened and read frames
bool testCamera(const string& device_path) {
    VideoCapture cap;
    if (device_path.find("/dev/") == 0) {
        cap.open(device_path, CAP_V4L2);
    } else {
        cap.open(stoi(device_path), CAP_V4L2);
    }
    
    if (!cap.isOpened()) {
        return false;
    }
    
    // Set basic properties
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 30);
    
    // Try to read a frame
    Mat testFrame;
    bool success = cap.read(testFrame);
    cap.release();
    
    return success && !testFrame.empty();
}

// Touch gesture detection
struct TouchGesture {
    int startX, startY;
    int lastX, lastY;
    bool tracking;
    long startTime;
};

void handleTouch(TouchGesture& gesture, int event, int x, int y, MirrorState& state) {
    long currentTime = getTickCount() * 1000 / getTickFrequency();
    
    if (event == EVENT_LBUTTONDOWN) {
        gesture.startX = x;
        gesture.startY = y;
        gesture.lastX = x;
        gesture.lastY = y;
        gesture.tracking = true;
        gesture.startTime = currentTime;
    }
    else if (event == EVENT_MOUSEMOVE && gesture.tracking) {
        gesture.lastX = x;
        gesture.lastY = y;
    }
    else if (event == EVENT_LBUTTONUP && gesture.tracking) {
        gesture.tracking = false;
        
        int deltaX = x - gesture.startX;
        int deltaY = y - gesture.startY;
        long duration = currentTime - gesture.startTime;
        
        // Only consider horizontal swipes (min 100px movement in max 500ms)
        if (abs(deltaX) > 100 && abs(deltaY) < 100 && duration < 500) {
            if (state.availableCameras >= 2 && state.mode == DUAL) {
                // In dual mode with 2 cameras
                if (deltaX > 0) {
                    state.mode = SINGLE;
                    state.currentCam = (x < 640) ? 0 : 1;
                    cout << "Switched to SINGLE mode with camera: " 
                         << (state.currentCam == 0 ? "Front" : "Rear") << endl;
                } else {
                    state.mode = SINGLE;
                    state.currentCam = (x < 640) ? 1 : 0;
                    cout << "Switched to SINGLE mode with camera: " 
                         << (state.currentCam == 0 ? "Front" : "Rear") << endl;
                }
            } else if (state.availableCameras == 1 && state.mode == SINGLE) {
                // In single mode with 1 camera - toggle between different views
                if (deltaX > 0) {
                    // Swipe right: switch view mode
                    state.mode = AUTO;
                    cout << "Switched to AUTO view mode" << endl;
                } else {
                    // Swipe left: return to single mode
                    state.mode = SINGLE;
                    cout << "Returned to SINGLE mode" << endl;
                }
            } else if (state.mode == AUTO) {
                // In auto mode - return to single
                state.mode = SINGLE;
                cout << "Returned to SINGLE mode" << endl;
            }
            state.needsRedraw = true;
        }
        else if (abs(deltaX) < 50 && abs(deltaY) < 50) {
            // Simple tap
            if (state.availableCameras >= 2 && state.mode == DUAL) {
                state.mode = SINGLE;
                state.currentCam = (x < 640) ? 0 : 1;
                cout << "Tapped camera: " 
                     << (state.currentCam == 0 ? "Front" : "Rear") 
                     << " - Switched to SINGLE mode" << endl;
            } else if (state.availableCameras == 1) {
                // Single camera - tap to toggle between normal and mirrored view
                state.mode = (state.mode == SINGLE) ? AUTO : SINGLE;
                cout << "Toggled view mode" << endl;
            }
            state.needsRedraw = true;
        }
    }
}

void drawUI(Mat& display, const MirrorState& state) {
    string modeText;
    Scalar color;
    
    if (state.availableCameras == 0) {
        modeText = "NO CAMERAS FOUND";
        color = Scalar(0, 0, 255);
    } else if (state.availableCameras == 1) {
        if (state.mode == SINGLE) {
            modeText = "SINGLE CAMERA VIEW";
            color = Scalar(0, 255, 255);
        } else {
            modeText = "MIRRORED VIEW";
            color = Scalar(255, 255, 0);
        }
        putText(display, "Tap to toggle view mode", Point(10, 60), 
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
    } else {
        if (state.mode == DUAL) {
            modeText = "DUAL CAMERA VIEW";
            color = Scalar(0, 255, 0);
            putText(display, "Front Camera", Point(240, 460), 
                    FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
            putText(display, "Rear Camera", Point(880, 460), 
                    FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
        } else {
            string camName = (state.currentCam == 0) ? "Front Camera" : "Rear Camera";
            modeText = "SINGLE VIEW - " + camName;
            color = Scalar(0, 255, 255);
            putText(display, "Swipe left for DUAL view", Point(10, 60), 
                    FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
            putText(display, "Swipe right to switch camera", Point(10, 90), 
                    FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
        }
    }
    
    putText(display, modeText, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, color, 2);
    putText(display, "Cameras available: " + to_string(state.availableCameras), 
            Point(10, display.rows - 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
    
    if (state.mode == DUAL) {
        line(display, Point(640, 0), Point(640, 480), Scalar(255, 255, 255), 2);
    }
}

int main() {
    cout << "Starting AI ADAS Smart Mirror (Flexible Camera Support)..." << endl;
    
    // Detect available V4L2 cameras
    auto camera_devices = detectV4L2Cameras();
    
    // Test each camera to see if it actually works
    vector<bool> camera_works(camera_devices.size(), false);
    vector<VideoCapture> cameras;
    
    for (size_t i = 0; i < camera_devices.size(); i++) {
        cout << "Testing camera " << i << " (" << camera_devices[i] << ")..." << endl;
        if (testCamera(camera_devices[i])) {
            camera_works[i] = true;
            cout << "✓ Camera " << i << " works!" << endl;
        } else {
            cout << "✗ Camera " << i << " failed to initialize" << endl;
        }
    }
    
    // Open only working cameras
    int working_cameras = 0;
    for (size_t i = 0; i < camera_devices.size(); i++) {
        if (camera_works[i]) {
            VideoCapture cap;
            if (camera_devices[i].find("/dev/") == 0) {
                cap.open(camera_devices[i], CAP_V4L2);
            } else {
                cap.open(stoi(camera_devices[i]), CAP_V4L2);
            }
            
            if (cap.isOpened()) {
                // Optimize V4L2 camera settings
                cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
                cap.set(CAP_PROP_FRAME_WIDTH, 640);
                cap.set(CAP_PROP_FRAME_HEIGHT, 480);
                cap.set(CAP_PROP_FPS, 30);
                cap.set(CAP_PROP_BUFFERSIZE, 2);
                
                cameras.push_back(cap);
                working_cameras++;
                cout << "Successfully opened camera " << working_cameras << ": " << camera_devices[i] << endl;
            }
        }
    }
    
    if (working_cameras == 0) {
        cerr << "No working cameras found!" << endl;
        return -1;
    }
    
    cout << "Successfully opened " << working_cameras << " camera(s)" << endl;

    // Wait for display to be ready
    sleep(2);

    namedWindow("AI ADAS Smart Mirror", WINDOW_NORMAL);
    setWindowProperty("AI ADAS Smart Mirror", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    MirrorState state;
    state.mode = (working_cameras >= 2) ? DUAL : SINGLE;
    state.currentCam = 0;
    state.needsRedraw = true;
    state.availableCameras = working_cameras;
    
    TouchGesture gesture;

    setMouseCallback("AI ADAS Smart Mirror", [](int event, int x, int y, int, void* userdata) {
        auto* data = reinterpret_cast<pair<MirrorState*, TouchGesture*>*>(userdata);
        handleTouch(*(data->second), event, x, y, *(data->first));
    }, new pair<MirrorState*, TouchGesture*>(&state, &gesture));

    cout << "AI ADAS Smart Mirror started with " << working_cameras << " camera(s)!" << endl;
    cout << "Touch Controls:" << endl;
    if (working_cameras >= 2) {
        cout << "- DUAL mode: Tap camera to view single, Swipe left/right to switch" << endl;
        cout << "- SINGLE mode: Swipe left for dual view, Swipe right to switch camera" << endl;
    } else {
        cout << "- Tap to toggle between normal and mirrored view" << endl;
        cout << "- Swipe to change view modes" << endl;
    }

    Mat display;
    int reset_count = 0;
    vector<Mat> frames(cameras.size());
    
    while (true) {
        // Read frames from all available cameras
        bool all_empty = true;
        for (size_t i = 0; i < cameras.size(); i++) {
            cameras[i] >> frames[i];
            if (!frames[i].empty()) {
                all_empty = false;
            }
        }
        
        if (all_empty) {
            cerr << "All camera frames empty!" << endl;
            reset_count++;
            if (reset_count > 30) {
                cerr << "Resetting cameras..." << endl;
                // Reopen all cameras
                for (auto& cap : cameras) {
                    cap.release();
                }
                cameras.clear();
                sleep(2);
                
                // Reopen cameras
                for (size_t i = 0; i < camera_devices.size(); i++) {
                    if (camera_works[i]) {
                        VideoCapture cap;
                        if (camera_devices[i].find("/dev/") == 0) {
                            cap.open(camera_devices[i], CAP_V4L2);
                        } else {
                            cap.open(stoi(camera_devices[i]), CAP_V4L2);
                        }
                        if (cap.isOpened()) {
                            cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
                            cap.set(CAP_PROP_FRAME_WIDTH, 640);
                            cap.set(CAP_PROP_FRAME_HEIGHT, 480);
                            cameras.push_back(cap);
                        }
                    }
                }
                reset_count = 0;
            }
            continue;
        }
        
        reset_count = 0;

        // Compose display based on current mode and available cameras
        if (state.availableCameras >= 2 && state.mode == DUAL) {
            // Dual camera view
            Mat f_resized, r_resized;
            resize(frames[0], f_resized, Size(640, 480));
            resize(frames[1], r_resized, Size(640, 480));
            hconcat(f_resized, r_resized, display);
        } else if (state.availableCameras >= 2 && state.mode == SINGLE) {
            // Single camera view (from 2 available)
            if (state.currentCam < (int)frames.size() && !frames[state.currentCam].empty()) {
                resize(frames[state.currentCam], display, Size(1280, 720));
            } else {
                // Fallback to first camera
                resize(frames[0], display, Size(1280, 720));
            }
        } else if (state.availableCameras == 1) {
            // Single camera with different view modes
            if (state.mode == SINGLE) {
                // Normal view
                resize(frames[0], display, Size(1280, 720));
            } else {
                // Mirrored view (split screen with same camera)
                Mat mirrored;
                flip(frames[0], mirrored, 1); // Horizontal flip
                Mat left_resized, right_resized;
                resize(frames[0], left_resized, Size(640, 480));
                resize(mirrored, right_resized, Size(640, 480));
                hconcat(left_resized, right_resized, display);
            }
        } else {
            // No cameras - show error display
            display = Mat::zeros(480, 640, CV_8UC3);
            putText(display, "NO CAMERAS AVAILABLE", Point(120, 240), 
                    FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
        }

        // Draw UI overlays
        drawUI(display, state);

        imshow("AI ADAS Smart Mirror", display);
        
        if (waitKey(1) >= 0) {
            break;
        }
    }

    // Release all cameras
    for (auto& cap : cameras) {
        cap.release();
    }
    destroyAllWindows();
    return 0;
}