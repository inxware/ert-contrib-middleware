#ifndef __OPENCV_WRAPPER__
#define __OPENCV_WRAPPER__

#ifdef __cplusplus
extern "C" {
#endif

// Equivalent definitions for OpenCV types
#define CV_TYPE_8UC1   0    // 8-bit unsigned, 1 channel
#define CV_TYPE_8UC3   16   // 8-bit unsigned, 3 channels (BGR)
#define CV_TYPE_32FC1  5    // 32-bit float, 1 channel
#define CV_TYPE_32FC3  21   // 32-bit float, 3 channels (RGB or similar)
#define CV_TYPE_32FC4  22   // 32-bit float, 4 channels (RGBA)

/* -----------------------------------------------------------
   Opaque handles (they are just pointers on the C side)
   ----------------------------------------------------------- */
typedef struct cv_camera { 
    void* impl;
    int libcamera_support_enabled;
    int libcamera_support_cap_timeout_ms; // capture timeout
} cv_camera;
/* cv_mat now embeds dimensions & channels */
typedef struct cv_mat {
    void* impl;     /* shared_ptr<cv::Mat> on the C++ side   */
    int   width;    /* cols  */
    int   height;   /* rows  */
    int   channels; /* always 3 (BGR) for this wrapper       */
} cv_mat;

/* Return codes */
enum cv_cam_err {
    CV_CAM_OK          = 0,
    CV_CAM_OPEN_ERR    = 1,
    CV_CAM_READ_ERR    = 2,
    CV_CAM_ALLOC_ERR   = 3,
    CV_CAM_WRITE_ERR   = 4
};

int  cv_cam_enable_libcamera_mode (cv_camera* cam, int enable, int cap_timeout_ms);

int  cv_cam_libcamera_support();

/* ---------------- Camera lifecycle ---------------- */
int  cv_cam_open (cv_camera* cam,
                  int device_id,
                  int width,
                  int height,
                  int fps,
                  int buffer_size,
                  bool monochrome,
                  int async_mode);
/* Read from a video file instead of camera */
int  cv_cam_path (cv_camera* cam,
                  const char* path,
                  int width,
                  int height,
                  int fps,
                  int buffer_size,
                  bool is_monochrome,
                  int async_mode);

void cv_cam_close(cv_camera* cam);

/* ---------------- Frame acquisition (zero-copy cv::Mat) ---------
   Fills *out_mat with an opaque handle referencing the same data
   OpenCV captured.  Call cv_mat_release(&m) when done.             */
int  cv_cam_read(cv_camera* cam, cv_mat* mat);

/* Reads an image from a file and stores it in a cv_mat structure. */
int cv_mat_read(const char* filename, cv_mat* dst);

/* Writes the contents of a cv_mat structure to an image file. */
int cv_mat_write(const char* filename, const cv_mat* src);

/* Release a cv_mat obtained from cv_cam_read_mat.                  */
void cv_mat_release(cv_mat* mat);

/* Get pointer to raw pixel data (valid as long as cv_mat is alive).
 * This may point to uint8_t*, float*, etc. depending on mat type.
 */
void* cv_mat_data(const cv_mat* mat);

/* Get total size in bytes = rows * cols * elemSize(). Useful for memcpy etc. */
unsigned int cv_mat_size_bytes(const cv_mat* mat);

/* Convert image type (e.g. to CV_32FC3), with optional scaling and shifting.
 * dst will be a newly allocated cv_mat; must be released with cv_mat_release().
 */
int cv_mat_convert_to(const cv_mat* src,
                      cv_mat* dst,
                      int rtype,
                      double alpha,
                      double beta);  // usually 0.0

/* Resize the image to target size.
 * src: input cv_mat
 * dst: output resized cv_mat
 * Caller must release dst using cv_mat_release.
 * Returns CV_CAM_OK or CV_CAM_ALLOC_ERR.
 */
int cv_mat_resize(const cv_mat* src, cv_mat* dst, int target_width, int target_height);

/*  Crop an OpenCV matrix.
 *  src            : source cv_mat (must point to a valid cv::Mat)
 *  dst            : destination cv_mat (receives a new shared_ptr<cv::Mat>)
 *  x, y           : top-left corner of crop (can be negative or outside)
 *  crop_w, crop_h : requested crop size (pixels)
 *
 *  Returns CV_CAM_OK on success, error code otherwise.
 */
int cv_mat_crop(const cv_mat* src, cv_mat* dst, int x, int y, int crop_w, int crop_h);

/* Convert to grayscale.
 * src must be 3-channel BGR image.
 * dst will be 1-channel grayscale image.
 * Returns CV_CAM_OK or CV_CAM_ALLOC_ERR.
 */
int cv_mat_to_grayscale(const cv_mat* src, cv_mat* dst);

/* Draws a rectangle on the given image (in-place).
 * (x, y) - top-left corner
 * (width, height) - dimensions of the rectangle
 * (r, g, b) - color (0-255 per channel)
 * thickness - line thickness (-1 to fill)
 * Returns CV_CAM_OK on success.
 */
int cv_mat_draw_rectangle(cv_mat* mat,
                          int x, int y,
                          int width, int height,
                          int r, int g, int b,
                          int thickness);

/**
 * Draw text on the image at (x, y).
 * mat         Target image (cv_mat).
 * text        Null-terminated C string to draw.
 * x, y        Bottom-left corner of the text.
 * font_scale  Font scale (e.g., 0.5, 1.0, etc.)
 * r, g, b     Text color in RGB.
 * thickness   Text thickness (e.g., 1 or 2).
 * CV_CAM_OK on success, or error code.
 */
int cv_mat_draw_text(cv_mat* mat,
                     const char* text,
                     int x, int y,
                     double font_scale,
                     int r, int g, int b,
                     int thickness);

/* Show a cv_mat in a window. The window stays open until key is pressed.
 * window_name : null-terminated string for window title
 * mat         : input frame (must be valid)
 * wait_ms     : delay in milliseconds for key input (0 = wait indefinitely)
 * Returns CV_CAM_OK on success, error code otherwise.
 */
int cv_mat_show(const char* window_name, const cv_mat* mat, int wait_ms);

/* Destroy all OpenCV windows created with cv_mat_show().
 * Safe to call even if no windows are open.
 */
void cv_mat_destroy_all_windows();


#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif /* __OPENCV_WRAPPER__ */
