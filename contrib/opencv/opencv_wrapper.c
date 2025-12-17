#include "opencv_wrapper.h"
#include <opencv2/opencv.hpp>
#ifdef CV_LIBCAMERA_SUPPORT
#include <lccv.hpp>
#include <libcamera_app.hpp>
#endif // CV_LIBCAMERA_SUPPORT
#include <memory>
#include <cstring>
#include <new>
#include <thread>
#include <mutex>
#include <atomic>

/* ---------- tiny RAII VideoCapture wrapper ------------------ */
class CameraWrapper {
public:
    bool open(int id, int w, int h, int fps, int buffer_size, bool monochrome, bool async_mode) {
        if (libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
            if (cam_ || running_ || !setup_libcam(id, w, h, fps, monochrome)) return false;
            if (cam_ && !cam_->startVideo()) return false;
#endif // CV_LIBCAMERA_SUPPORT
        }else{
            if (running_ || cap_.isOpened()) return false; // already running, make sure to close it first
            if (!cap_.open(id)) return false;
        }
        return setup(w, h, fps, buffer_size, monochrome, async_mode);
    }
    bool open(const char* path, int w, int h, int fps, int buffer_size, bool monochrome, bool async_mode) {
        if (!path) return false;
        if (libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
            // libcamera only use integer based id
            if (running_) return false;
#endif // CV_LIBCAMERA_SUPPORT
        }else{
            if (running_ || cap_.isOpened()) return false; // already running, make sure to close it first
            if (!cap_.open(path)) return false;
        }
        return setup(w, h, fps, buffer_size, monochrome, async_mode);
    }
    void close()            { stop_async(); release(); }
    bool read(cv::Mat& out) { return (!async_mode_) ? read_normal(out) : read_async(out); }
    void enable_libcamera(bool enable, int timeout=1000) { libcamera_enabled_ = enable; libcamera_timeout_ = timeout; }
private:
    void release()
    {
        if(libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
            if(cam_) { cam_->stopVideo(); cam_ = nullptr; }
#endif // CV_LIBCAMERA_SUPPORT
        }else{
            cap_.release();
        }
    }
    bool setup(int w, int h, int fps, int buffer_size, bool monochrome, bool async_mode) {
        bool is_open = false;
        if(libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
            is_open = cam_ != nullptr;
#endif // CV_LIBCAMERA_SUPPORT
        }else{
            if (w  > 0) cap_.set(cv::CAP_PROP_FRAME_WIDTH,  w);
            if (h  > 0) cap_.set(cv::CAP_PROP_FRAME_HEIGHT, h);
            if (fps > 0) cap_.set(cv::CAP_PROP_FPS,         fps);
            if (monochrome) cap_.set(cv::CAP_PROP_MONOCHROME, 100);
            if (buffer_size > 0) cap_.set(cv::CAP_PROP_BUFFERSIZE, buffer_size);
            is_open = cap_.isOpened();
        }
        async_mode_ = async_mode;
        if (is_open && async_mode_){
            is_open = start_async();
            if(!is_open) close();
        }
        return is_open;
    }
#ifdef CV_LIBCAMERA_SUPPORT
    bool setup_libcam(int id, int w, int h, int fps, bool is_monochrome){
        cam_ = std::make_unique<lccv::PiCamera>(id);
        if(!cam_) return false;
        if(w > 0) cam_->options->video_width=w;
        if(h > 0) cam_->options->video_height=h;
        if(fps > 0) cam_->options->framerate=fps;
        cam_->options->saturation = is_monochrome ? 0 : 1;
        //cam_->options->verbose=true;
        return true;
    }
#endif
    bool start_async() {
        if (running_) return false;
        running_ = true;
        thread_ = std::thread(&CameraWrapper::grab_loop, this);
        return true;
    }
    void stop_async() {
        if (async_mode_ && running_) {
            running_ = false;
            if (thread_.joinable()) {
                thread_.join();
            }
            std::lock_guard<std::mutex> lk(mutex_);
            frame_.release(); // Clear any leftover frame
        }
        running_=false;
    }
    bool read_async(cv::Mat& out) {
        if (!running_) return false;
        std::lock_guard<std::mutex> lk(mutex_);
        if (frame_.empty()) return false;
        out = frame_.clone();
        return true; 
    }
    bool read_normal(cv::Mat& out) {
        if(libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
            return (cam_) ? cam_->getVideoFrame(out, libcamera_timeout_) : false;
#endif // CV_LIBCAMERA_SUPPORT
        }
        return cap_.read(out);
    }

    void grab_loop() {
        cv::Mat frame;
        while (running_) {
            bool ret = false;
            if(libcamera_enabled_){
#ifdef CV_LIBCAMERA_SUPPORT
                ret = (cam_) ? cam_->getVideoFrame(frame, libcamera_timeout_) : false;
#endif // CV_LIBCAMERA_SUPPORT
            } else {
                ret = cap_.read(frame);
            }
            if (!ret) {
                continue;
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                frame.copyTo(frame_);
            }
            // @TODO - add callback for async mode
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#ifdef CV_LIBCAMERA_SUPPORT
    std::unique_ptr<lccv::PiCamera> cam_=nullptr;
#endif // CV_LIBCAMERA_SUPPORT
    cv::VideoCapture cap_;
    bool async_mode_=false;
    bool libcamera_enabled_=false;
    std::atomic<int> libcamera_timeout_=1000;
    std::atomic<bool> running_=false;
    std::mutex mutex_;
    cv::Mat frame_;      // async frame
    std::thread thread_; // async thread
};

/* ---------- helpers to cast opaque C handles ---------------- */
static inline CameraWrapper* to_cpp(cv_camera* c) {
    return c ? static_cast<CameraWrapper*>(c->impl) : nullptr;
}

/* A cv_mat is a heap-allocated shared_ptr<cv::Mat>.
   This lets many C functions share the same underlying pixels.   */
using MatPtr = std::shared_ptr<cv::Mat>;
static inline MatPtr* to_mat(cv_mat* m) {
    return m ? static_cast<MatPtr*>(m->impl) : nullptr;
}

/* ============================================================ *
 *                      C  API  STARTS HERE                     *
 * ============================================================ */
extern "C" {

int  cv_cam_enable_libcamera_mode (cv_camera* cam, int enable, int cap_timeout_ms)
{
    if (!cam) return -1;
#ifdef CV_LIBCAMERA_SUPPORT
    cam->libcamera_support_enabled = enable;
    if(cap_timeout_ms > 0) cam->libcamera_support_cap_timeout_ms = cap_timeout_ms;
#else
    cam->libcamera_support_enabled = 0;
    return 1;
#endif // CV_LIBCAMERA_SUPPORT
    return 0;
}

int  cv_cam_libcamera_support()
{
#ifdef CV_LIBCAMERA_SUPPORT
    return 1;
#endif // CV_LIBCAMERA_SUPPORT
    return 0;
}

int cv_cam_open(cv_camera* cam, int id, int w, int h, int fps, int buffer_size, bool is_monochrome, int async_mode)
{
    if (!cam) return CV_CAM_OPEN_ERR;
    std::unique_ptr<CameraWrapper> cpp(new (std::nothrow) CameraWrapper);
    if (!cpp)                       return CV_CAM_ALLOC_ERR;
    if (cv_cam_libcamera_support()) cpp->enable_libcamera(cam->libcamera_support_enabled, cam->libcamera_support_cap_timeout_ms);
    if (!cpp->open(id, w, h, fps, buffer_size, is_monochrome, (async_mode > 0)))  return CV_CAM_OPEN_ERR;
    cam->impl = cpp.release();
    return CV_CAM_OK;
}

int  cv_cam_path (cv_camera* cam, const char* path, int w, int h, int fps, int buffer_size, bool is_monochrome, int async_mode)
{
    if (!cam) return CV_CAM_OPEN_ERR;
    std::unique_ptr<CameraWrapper> cpp(new (std::nothrow) CameraWrapper);
    if (!cpp)                       return CV_CAM_ALLOC_ERR;
    if (cv_cam_libcamera_support()) cpp->enable_libcamera(cam->libcamera_support_enabled, cam->libcamera_support_cap_timeout_ms);
    if (!cpp->open(path, w, h, fps, buffer_size, is_monochrome, (async_mode > 0)))  return CV_CAM_OPEN_ERR;
    cam->impl = cpp.release();
    return CV_CAM_OK;
}

void cv_cam_close(cv_camera* cam)
{
    if (auto* cpp = to_cpp(cam)) {
        cpp->close();
        delete cpp;
        cam->impl = nullptr;
    }
}

/* ---------- zero-copy cv::Mat path --------------------------- */
int cv_cam_read(cv_camera* cam,
                cv_mat* m)
{
    if (!cam || !m) return CV_CAM_READ_ERR;
    CameraWrapper* cpp = to_cpp(cam);
    if (!cpp) return CV_CAM_READ_ERR;

    cv::Mat frame;
    if (!cpp->read(frame)) return CV_CAM_READ_ERR;

    /* create a shared_ptr on heap so it survives after function returns */
    MatPtr* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(frame)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    m->impl = static_cast<void*>(holder);
    m->width  = (*holder)->cols;
    m->height  = (*holder)->rows;
    m->channels = 3;
    return CV_CAM_OK;
}

int cv_mat_read(const char* filename, cv_mat* dst)
{
    if (!filename || !dst) return CV_CAM_READ_ERR;

    cv::Mat img = cv::imread(filename, cv::IMREAD_UNCHANGED);
    if (img.empty()) return CV_CAM_READ_ERR;

    auto* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(img)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    dst->impl = holder;
    dst->width = (*holder)->cols;
    dst->height = (*holder)->rows;
    dst->channels = (*holder)->channels();

    return CV_CAM_OK;
}

int cv_mat_write(const char* filename, const cv_mat* src)
{
    if (!filename || !src || !src->impl) return CV_CAM_WRITE_ERR;

    auto sp_src = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(src->impl));
    if (!sp_src || sp_src->empty()) return CV_CAM_WRITE_ERR;

    bool success = cv::imwrite(filename, *sp_src);
    return success ? CV_CAM_OK : CV_CAM_WRITE_ERR;
}

void cv_mat_release(cv_mat* m)
{
    MatPtr* holder = to_mat(m);
    if (holder) {
        delete holder;           /* drops shared_ptr refcount */
        m->impl = nullptr;
    }
}

void* cv_mat_data(const cv_mat* mat)
{
    if (!mat || !mat->impl) return nullptr;
    auto sp = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(mat->impl));
    if (!sp || sp->empty()) return nullptr;
    return static_cast<void*>(sp->data);
}

unsigned int cv_mat_size_bytes(const cv_mat* mat)
{
    if (!mat || !mat->impl) return 0;
    auto sp = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(mat->impl));
    if (!sp || sp->empty()) return 0;
    return static_cast<unsigned int>( sp->total() * sp->elemSize() );
}

int cv_mat_convert_to(const cv_mat* src,
                      cv_mat* dst,
                      int rtype,
                      double alpha,
                      double beta)
{
    if (!src || !src->impl || !dst) return CV_CAM_READ_ERR;

    auto sp_src = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(src->impl));
    if (!sp_src || sp_src->empty()) return CV_CAM_READ_ERR;

    cv::Mat converted;
    sp_src->convertTo(converted, rtype, alpha, beta);

    auto* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(converted)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    dst->impl = holder;
    dst->width = holder->get()->cols;
    dst->height = holder->get()->rows;
    dst->channels = holder->get()->channels();

    return CV_CAM_OK;
}

int cv_mat_resize(const cv_mat* src, cv_mat* dst, int target_width, int target_height)
{
    if (!src || !src->impl || !dst) return CV_CAM_READ_ERR;

    auto sp_src = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(src->impl));
    if (!sp_src || sp_src->empty()) return CV_CAM_READ_ERR;

    cv::Mat resized;
    cv::resize(*sp_src, resized, cv::Size(target_width, target_height));

    auto* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(resized)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    dst->impl = holder;
    dst->width = target_width;
    dst->height = target_height;
    dst->channels = sp_src->channels();  // Usually 3

    return CV_CAM_OK;
}

int cv_mat_crop(const cv_mat* src,
                cv_mat*       dst,
                int           x,
                int           y,
                int           crop_w,
                int           crop_h)
{
    if (!src || !src->impl || !dst) return CV_CAM_READ_ERR;

    /* dereference the shared_ptr<cv::Mat> stored in src->impl */
    auto sp_src = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(src->impl));
    if (!sp_src || sp_src->empty()) return CV_CAM_READ_ERR;

    /* ---- clamp crop rectangle to the source image -------------------- */
    int src_w = sp_src->cols;
    int src_h = sp_src->rows;

    /* clip the requested rectangle */
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(x + crop_w, src_w);
    int y1 = std::min(y + crop_h, src_h);

    int w = x1 - x0;
    int h = y1 - y0;
    if (w <= 0 || h <= 0) return CV_CAM_READ_ERR;   /* nothing to crop */

    /* ---- perform the crop (ROI) ------------------------------------- */
    cv::Rect roi(x0, y0, w, h);
    cv::Mat cropped = (*sp_src)(roi).clone();       /* clone → own buffer */

    /* ---- wrap in shared_ptr and store in dst ------------------------ */
    auto* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(cropped)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    dst->impl     = holder;
    dst->width    = w;
    dst->height   = h;
    dst->channels = sp_src->channels();             /* usually 3 */

    return CV_CAM_OK;
}


int cv_mat_to_grayscale(const cv_mat* src, cv_mat* dst)
{
    if (!src || !src->impl || !dst) return CV_CAM_READ_ERR;

    auto sp_src = *(reinterpret_cast<const std::shared_ptr<cv::Mat>*>(src->impl));
    if (!sp_src || sp_src->channels() != 3) return CV_CAM_READ_ERR;

    cv::Mat gray;
    cv::cvtColor(*sp_src, gray, cv::COLOR_BGR2GRAY);

    auto* holder = new (std::nothrow) MatPtr(std::make_shared<cv::Mat>(std::move(gray)));
    if (!holder) return CV_CAM_ALLOC_ERR;

    dst->impl = holder;
    dst->width = holder->get()->cols;
    dst->height = holder->get()->rows;
    dst->channels = 1;

    return CV_CAM_OK;
}

int cv_mat_draw_rectangle(cv_mat* mat,
                          int x, int y,
                          int width, int height,
                          int r, int g, int b,
                          int thickness)
{
    if (!mat || !mat->impl) {
        return CV_CAM_ALLOC_ERR;
    }

    auto mat_ptr = static_cast<std::shared_ptr<cv::Mat>*>(mat->impl);
    if (!mat_ptr || !(*mat_ptr) || (*mat_ptr)->empty()) {
        return CV_CAM_ALLOC_ERR;
    }

    try {
        cv::rectangle(*(*mat_ptr),
                      cv::Rect(x, y, width, height),
                      cv::Scalar(b, g, r),  // OpenCV uses BGR order
                      thickness);
        return CV_CAM_OK;
    } catch (...) {
        return CV_CAM_ALLOC_ERR;
    }
}

int cv_mat_draw_text(cv_mat* mat,
                     const char* text,
                     int x, int y,
                     double font_scale,
                     int r, int g, int b,
                     int thickness)
{
    if (!mat || !mat->impl || !text) return CV_CAM_ALLOC_ERR;

    auto mat_ptr = static_cast<std::shared_ptr<cv::Mat>*>(mat->impl);
    if (!mat_ptr || !(*mat_ptr) || (*mat_ptr)->empty()) return CV_CAM_ALLOC_ERR;

    try {
        cv::putText(*(*mat_ptr),
                    text,
                    cv::Point(x, y),
                    cv::FONT_HERSHEY_SIMPLEX,
                    font_scale,
                    cv::Scalar(b, g, r),
                    thickness);
        return CV_CAM_OK;
    } catch (...) {
        return CV_CAM_ALLOC_ERR;
    }
}


int cv_mat_show(const char* window_name, const cv_mat* mat, int wait_ms) {
    if (!window_name || !mat || !mat->impl) {
        return CV_CAM_ALLOC_ERR;
    }

    auto mat_ptr = static_cast<std::shared_ptr<cv::Mat>*>(mat->impl);
    if (!mat_ptr || !(*mat_ptr) || (*mat_ptr)->empty()) {
        return CV_CAM_ALLOC_ERR;
    }

    try {
        cv::imshow(window_name, *(*mat_ptr));
        cv::waitKey(wait_ms);
        return CV_CAM_OK;
    } catch (...) {
        return CV_CAM_ALLOC_ERR;
    }
}

void cv_mat_destroy_all_windows() {
    cv::destroyAllWindows();
}

} /* extern "C" */
