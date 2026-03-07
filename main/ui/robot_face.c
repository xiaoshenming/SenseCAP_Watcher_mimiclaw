#include "robot_face.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "robot_face";

/* ── Layout constants (412×412 circular display) ── */
#define SCR_W        412
#define SCR_H        412

#define EYE_W        80
#define EYE_H        80
#define PUPIL_W      26
#define PUPIL_H      26
#define HL_SZ        8     /* highlight dot size */

/* Eye positions (top-left corner) */
#define L_EYE_X      96
#define L_EYE_Y      135
#define R_EYE_X      236
#define R_EYE_Y      135

/* Pupil default center within eye */
#define PUPIL_CX     ((EYE_W - PUPIL_W) / 2)
#define PUPIL_CY     ((EYE_H - PUPIL_H) / 2)

/* Cheeks */
#define CHEEK_W      38
#define CHEEK_H      20
#define L_CHEEK_X    97
#define L_CHEEK_Y    235
#define R_CHEEK_X    277
#define R_CHEEK_Y    235

/* Mouth – small ellipse */
#define MOUTH_W      28
#define MOUTH_H      12
#define MOUTH_X      192
#define MOUTH_Y      270

/* Colors */
#define COL_BG        lv_color_hex(0x0f0f23)
#define COL_EYE       lv_color_white()
#define COL_PUPIL     lv_color_hex(0x2d2d44)
#define COL_HL        lv_color_white()
#define COL_CHEEK     lv_color_hex(0xFFB4C2)
#define COL_MOUTH     lv_color_hex(0xFF8A9E)

/* ── State ── */
static bool s_active = false;
static lv_obj_t *s_face = NULL;
static lv_obj_t *s_left_eye = NULL, *s_right_eye = NULL;
static lv_obj_t *s_left_pupil = NULL, *s_right_pupil = NULL;
static lv_timer_t *s_blink_timer = NULL;
static lv_timer_t *s_look_timer = NULL;
static int32_t s_pupil_x = PUPIL_CX;
static int32_t s_pupil_y = PUPIL_CY;
static int s_pupil_anim_tag;   /* dummy var for anim delete */

/* ── Animation exec callbacks ── */
static void set_height_cb(void *obj, int32_t h)
{
    lv_obj_set_height((lv_obj_t *)obj, h);
}

static void set_y_cb(void *obj, int32_t y)
{
    lv_obj_set_y((lv_obj_t *)obj, y);
}

static void set_translate_y_cb(void *obj, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t *)obj, v, 0);
}

static void set_pupil_x_cb(void *unused, int32_t x)
{
    if (s_left_pupil)  lv_obj_set_x(s_left_pupil, x);
    if (s_right_pupil) lv_obj_set_x(s_right_pupil, x);
    s_pupil_x = x;
}

static void set_pupil_y_cb(void *unused, int32_t y)
{
    if (s_left_pupil)  lv_obj_set_y(s_left_pupil, y);
    if (s_right_pupil) lv_obj_set_y(s_right_pupil, y);
    s_pupil_y = y;
}

/* ── Helper: styled rounded rectangle / circle ── */
static lv_obj_t *make_circle(lv_obj_t *parent, int x, int y,
                              int w, int h, lv_color_t color, lv_opa_t opa)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

/* ── Blink animation ── */
static void do_blink(lv_obj_t *eye, int ey)
{
    lv_anim_t ah;
    lv_anim_init(&ah);
    lv_anim_set_var(&ah, eye);
    lv_anim_set_values(&ah, EYE_H, 4);
    lv_anim_set_exec_cb(&ah, set_height_cb);
    lv_anim_set_duration(&ah, 80);
    lv_anim_set_playback_duration(&ah, 80);
    lv_anim_set_path_cb(&ah, lv_anim_path_ease_in_out);
    lv_anim_start(&ah);

    lv_anim_t ay;
    lv_anim_init(&ay);
    lv_anim_set_var(&ay, eye);
    lv_anim_set_values(&ay, ey, ey + (EYE_H - 4) / 2);
    lv_anim_set_exec_cb(&ay, set_y_cb);
    lv_anim_set_duration(&ay, 80);
    lv_anim_set_playback_duration(&ay, 80);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
    lv_anim_start(&ay);
}

static void blink_cb(lv_timer_t *t)
{
    do_blink(s_left_eye, L_EYE_Y);
    do_blink(s_right_eye, R_EYE_Y);
    lv_timer_set_period(t, 2000 + (esp_random() % 3000));
}

/* ── Look-around animation ── */
static const int8_t s_look[][2] = {
    { 0,  0}, {-12, 0}, { 12, 0}, { 0,-10}, { 0, 8},
    {-8, -8}, { 8, -8}, {-8,  6}, { 8, 6},
};

static void look_cb(lv_timer_t *t)
{
    int idx = esp_random() % (sizeof(s_look) / sizeof(s_look[0]));
    int32_t tx = PUPIL_CX + s_look[idx][0];
    int32_t ty = PUPIL_CY + s_look[idx][1];

    lv_anim_t ax;
    lv_anim_init(&ax);
    lv_anim_set_var(&ax, &s_pupil_anim_tag);
    lv_anim_set_values(&ax, s_pupil_x, tx);
    lv_anim_set_exec_cb(&ax, set_pupil_x_cb);
    lv_anim_set_duration(&ax, 300);
    lv_anim_set_path_cb(&ax, lv_anim_path_ease_in_out);
    lv_anim_start(&ax);

    lv_anim_t ay;
    lv_anim_init(&ay);
    lv_anim_set_var(&ay, &s_pupil_anim_tag);
    lv_anim_set_values(&ay, s_pupil_y, ty);
    lv_anim_set_exec_cb(&ay, set_pupil_y_cb);
    lv_anim_set_duration(&ay, 300);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
    lv_anim_start(&ay);

    lv_timer_set_period(t, 3000 + (esp_random() % 3000));
}

/* ── Breathing – gentle vertical float ── */
static void start_breathing(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_face);
    lv_anim_set_values(&a, 0, 4);
    lv_anim_set_exec_cb(&a, set_translate_y_cb);
    lv_anim_set_duration(&a, 2500);
    lv_anim_set_playback_duration(&a, 2500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ── Build the face scene ── */
static void create_face(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* transparent container for all face parts */
    s_face = lv_obj_create(scr);
    lv_obj_remove_flag(s_face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_face, SCR_W, SCR_H);
    lv_obj_set_pos(s_face, 0, 0);
    lv_obj_set_style_bg_opa(s_face, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_face, 0, 0);
    lv_obj_set_style_pad_all(s_face, 0, 0);

    /* eyes */
    s_left_eye  = make_circle(s_face, L_EYE_X, L_EYE_Y, EYE_W, EYE_H, COL_EYE, LV_OPA_COVER);
    s_right_eye = make_circle(s_face, R_EYE_X, R_EYE_Y, EYE_W, EYE_H, COL_EYE, LV_OPA_COVER);

    /* pupils */
    s_left_pupil  = make_circle(s_left_eye,  PUPIL_CX, PUPIL_CY, PUPIL_W, PUPIL_H, COL_PUPIL, LV_OPA_COVER);
    s_right_pupil = make_circle(s_right_eye, PUPIL_CX, PUPIL_CY, PUPIL_W, PUPIL_H, COL_PUPIL, LV_OPA_COVER);

    /* eye highlights (catchlight) */
    make_circle(s_left_pupil,  PUPIL_W - HL_SZ - 2, 2, HL_SZ, HL_SZ, COL_HL, LV_OPA_COVER);
    make_circle(s_right_pupil, PUPIL_W - HL_SZ - 2, 2, HL_SZ, HL_SZ, COL_HL, LV_OPA_COVER);

    /* cheeks (blush) */
    make_circle(s_face, L_CHEEK_X, L_CHEEK_Y, CHEEK_W, CHEEK_H, COL_CHEEK, LV_OPA_70);
    make_circle(s_face, R_CHEEK_X, R_CHEEK_Y, CHEEK_W, CHEEK_H, COL_CHEEK, LV_OPA_70);

    /* mouth */
    make_circle(s_face, MOUTH_X, MOUTH_Y, MOUTH_W, MOUTH_H, COL_MOUTH, LV_OPA_COVER);
}

/* ══════════════════════════════════════════════════
 *  Public API
 * ══════════════════════════════════════════════════ */

esp_err_t robot_face_start(void)
{
    if (s_active) return ESP_OK;

    if (!lvgl_port_lock(500)) {
        ESP_LOGE(TAG, "LVGL lock timeout");
        return ESP_FAIL;
    }

    create_face();

    s_blink_timer = lv_timer_create(blink_cb, 3000, NULL);
    s_look_timer  = lv_timer_create(look_cb,  4000, NULL);
    start_breathing();

    lvgl_port_unlock();

    s_active = true;
    ESP_LOGI(TAG, "Robot face started 🤖");
    return ESP_OK;
}

esp_err_t robot_face_stop(void)
{
    if (!s_active) return ESP_OK;

    if (!lvgl_port_lock(500)) {
        ESP_LOGE(TAG, "LVGL lock timeout");
        return ESP_FAIL;
    }

    /* stop timers */
    if (s_blink_timer) { lv_timer_delete(s_blink_timer); s_blink_timer = NULL; }
    if (s_look_timer)  { lv_timer_delete(s_look_timer);  s_look_timer  = NULL; }

    /* cancel running animations */
    lv_anim_delete(s_face, NULL);
    lv_anim_delete(s_left_eye, NULL);
    lv_anim_delete(s_right_eye, NULL);
    lv_anim_delete(&s_pupil_anim_tag, NULL);

    /* delete the face container (and all children) */
    if (s_face && lv_obj_is_valid(s_face)) {
        lv_obj_delete(s_face);
    }
    s_face = s_left_eye = s_right_eye = NULL;
    s_left_pupil = s_right_pupil = NULL;
    s_pupil_x = PUPIL_CX;
    s_pupil_y = PUPIL_CY;

    /* restore screen to black */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    lvgl_port_unlock();

    s_active = false;
    ESP_LOGI(TAG, "Robot face stopped");
    return ESP_OK;
}

bool robot_face_is_active(void)
{
    return s_active;
}
