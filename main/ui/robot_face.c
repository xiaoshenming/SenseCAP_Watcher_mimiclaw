#include "robot_face.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "robot_face";

/* ── Layout constants (412x412 round display) ── */
#define SCR_W        412
#define SCR_H        412
#define EYE_W        80
#define EYE_H        80
#define HL_SZ        8

#define L_EYE_X      96
#define L_EYE_Y      135
#define R_EYE_X      236
#define R_EYE_Y      135
#define L_EYE_CY     (L_EYE_Y + EYE_H / 2)
#define R_EYE_CY     (R_EYE_Y + EYE_H / 2)

#define CHEEK_W      38
#define CHEEK_H      20
#define L_CHEEK_X    97
#define L_CHEEK_Y    235
#define R_CHEEK_X    277
#define R_CHEEK_Y    235

#define COL_BG       lv_color_hex(0x0f0f23)
#define COL_EYE      lv_color_white()
#define COL_PUPIL    lv_color_hex(0x2d2d44)
#define COL_HL       lv_color_white()
#define COL_CHEEK    lv_color_hex(0xFFB4C2)

/* ── Emotion presets ── */
typedef struct {
    const char *name;
    const char *desc;
    int32_t l_eye_h, r_eye_h;
    int32_t mouth_w, mouth_h, mouth_y;
    uint32_t mouth_col;
    lv_opa_t cheek_opa;
    int32_t l_pdx, l_pdy;
    int32_t r_pdx, r_pdy;
    int32_t pupil_w, pupil_h;
    bool do_blink, do_look;
} emo_preset_t;

/*                     name          desc                     leh reh  mw  mh  my    mcol   chk  ldx ldy rdx rdy  pw  ph  blink look */
static const emo_preset_t EMOTIONS[] = {
    {"neutral",   "Default calm face",              80, 80,  28, 12, 270, 0xFF8A9E, 178,   0,  0,   0,  0, 26, 26, true,  true },
    {"happy",     "Joyful squinted eyes, smile",    25, 25,  52, 18, 265, 0xFF8A9E, 255,   0,  0,   0,  0, 26, 26, false, false},
    {"sad",       "Droopy eyes, tiny mouth",        72, 72,  18,  6, 280, 0x8888AA,   0,   0,  8,   0,  8, 26, 26, true,  false},
    {"angry",     "Intense squint, tight mouth",    52, 52,  36,  6, 272, 0xFF4444,   0,   0,  0,   0,  0, 18, 18, false, false},
    {"surprised", "Wide eyes, O-shaped mouth",      80, 80,  26, 26, 268, 0xFF8A9E,   0,   0,  0,   0,  0, 14, 14, false, false},
    {"love",      "Heart-eyed, big smile",          20, 20,  48, 16, 266, 0xFF6B8A, 255,   0,  0,   0,  0, 26, 26, false, false},
    {"sleepy",    "Half-closed drowsy eyes",        14, 14,  18,  8, 275, 0xFF8A9E,   0,   0,  0,   0,  0, 26, 26, false, false},
    {"wink",      "Left open, right closed",        80,  8,  44, 14, 266, 0xFF8A9E, 178,   0,  0,   0,  0, 26, 26, false, false},
    {"thinking",  "Eyes look up-left",              80, 80,  14, 10, 278, 0xFF8A9E,   0, -12,-10, -12,-10, 26, 26, false, false},
    {"excited",   "Big pupils, wide mouth",         80, 80,  56, 22, 260, 0xFF8A9E, 255,   0,  0,   0,  0, 30, 30, true,  true },
    {"scared",    "Wide eyes, tiny pupils",         80, 80,  22, 22, 270, 0xFFAAAA,   0,   0,  0,   0,  0, 12, 12, true,  false},
    {"cool",      "Sunglasses squint, smirk",       35, 35,  32,  8, 272, 0xFF8A9E,   0,   0,  0,   0,  0, 26, 20, false, false},
    {"cry",       "Sad eyes, tearful",              72, 72,  16,  5, 282, 0x8888AA,   0,   0, 10,   0, 10, 26, 26, false, false},
    {"dizzy",     "Cross-eyed confusion",           80, 80,  14, 14, 275, 0xFF8A9E,   0, -14,  0,  14,  0, 22, 22, false, false},
    {"shy",       "Slightly squinted, blushing",    60, 60,  22, 10, 272, 0xFF8A9E, 255,   6,  4,   6,  4, 22, 22, true,  false},
    {"smug",      "One eye squinted, smirk",        60, 40,  38, 10, 268, 0xFF8A9E,  80,   4,  0,   4,  0, 26, 26, false, false},
};
#define EMO_COUNT (sizeof(EMOTIONS) / sizeof(EMOTIONS[0]))

/* ── State ── */
static bool s_active = false;
static int s_emo_idx = 0;
static lv_obj_t *s_face = NULL;
static lv_obj_t *s_left_eye = NULL, *s_right_eye = NULL;
static lv_obj_t *s_left_pupil = NULL, *s_right_pupil = NULL;
static lv_obj_t *s_mouth = NULL;
static lv_obj_t *s_left_cheek = NULL, *s_right_cheek = NULL;
static lv_timer_t *s_blink_timer = NULL;
static lv_timer_t *s_look_timer = NULL;

/* ── Generic animation callbacks ── */
static void anim_height(void *o, int32_t v)  { lv_obj_set_height(o, v); }
static void anim_width(void *o, int32_t v)   { lv_obj_set_width(o, v); }
static void anim_y(void *o, int32_t v)       { lv_obj_set_y(o, v); }
static void anim_x(void *o, int32_t v)       { lv_obj_set_x(o, v); }
static void anim_bg_opa(void *o, int32_t v)  { lv_obj_set_style_bg_opa(o, (lv_opa_t)v, 0); }
static void anim_tr_y(void *o, int32_t v)    { lv_obj_set_style_translate_y(o, v, 0); }

/* ── Helper: create a styled rounded object ── */
static lv_obj_t *make_box(lv_obj_t *parent, int x, int y, int w, int h,
                           lv_color_t color, lv_opa_t opa)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_outline_width(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

/* Helper: animate one property smoothly */
static void anim_to(lv_obj_t *obj, lv_anim_exec_xcb_t cb,
                     int32_t from, int32_t to, uint32_t dur)
{
    if (from == to) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_set_duration(&a, dur);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ── Blink ── */
static void do_blink(lv_obj_t *eye, int32_t center_y)
{
    int32_t cur_h = lv_obj_get_height(eye);
    int32_t cur_y = lv_obj_get_y(eye);

    lv_anim_t ah; lv_anim_init(&ah);
    lv_anim_set_var(&ah, eye);
    lv_anim_set_values(&ah, cur_h, 4);
    lv_anim_set_exec_cb(&ah, anim_height);
    lv_anim_set_duration(&ah, 80);
    lv_anim_set_playback_duration(&ah, 80);
    lv_anim_set_path_cb(&ah, lv_anim_path_ease_in_out);
    lv_anim_start(&ah);

    lv_anim_t ay; lv_anim_init(&ay);
    lv_anim_set_var(&ay, eye);
    lv_anim_set_values(&ay, cur_y, center_y - 2);
    lv_anim_set_exec_cb(&ay, anim_y);
    lv_anim_set_duration(&ay, 80);
    lv_anim_set_playback_duration(&ay, 80);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
    lv_anim_start(&ay);
}

static void blink_cb(lv_timer_t *t)
{
    do_blink(s_left_eye, L_EYE_CY);
    do_blink(s_right_eye, R_EYE_CY);
    lv_timer_set_period(t, 2000 + (esp_random() % 3000));
}

/* ── Look around ── */
static void look_cb(lv_timer_t *t)
{
    const emo_preset_t *e = &EMOTIONS[s_emo_idx];
    static const int8_t offs[][2] = {
        {0,0},{-10,0},{10,0},{0,-8},{0,6},{-6,-6},{6,-6},{-6,4},{6,4}
    };
    int idx = esp_random() % 9;

    int32_t pw = lv_obj_get_width(s_left_pupil);
    int32_t ph = lv_obj_get_height(s_left_pupil);
    int32_t leh = lv_obj_get_height(s_left_eye);
    int32_t reh = lv_obj_get_height(s_right_eye);

    int32_t ltx = (EYE_W - pw)/2 + e->l_pdx + offs[idx][0];
    int32_t lty = (leh - ph)/2   + e->l_pdy + offs[idx][1];
    int32_t rtx = (EYE_W - pw)/2 + e->r_pdx + offs[idx][0];
    int32_t rty = (reh - ph)/2   + e->r_pdy + offs[idx][1];

    anim_to(s_left_pupil,  anim_x, lv_obj_get_x(s_left_pupil),  ltx, 300);
    anim_to(s_left_pupil,  anim_y, lv_obj_get_y(s_left_pupil),  lty, 300);
    anim_to(s_right_pupil, anim_x, lv_obj_get_x(s_right_pupil), rtx, 300);
    anim_to(s_right_pupil, anim_y, lv_obj_get_y(s_right_pupil), rty, 300);

    lv_timer_set_period(t, 3000 + (esp_random() % 3000));
}

/* ── Emotion transition ── */
static void apply_emotion(int idx, uint32_t dur)
{
    const emo_preset_t *e = &EMOTIONS[idx];

    /* Cancel all running property anims on face elements */
    lv_anim_delete(s_left_eye, NULL);
    lv_anim_delete(s_right_eye, NULL);
    lv_anim_delete(s_left_pupil, NULL);
    lv_anim_delete(s_right_pupil, NULL);
    lv_anim_delete(s_mouth, NULL);
    lv_anim_delete(s_left_cheek, NULL);
    lv_anim_delete(s_right_cheek, NULL);

    /* Eyes: animate height and keep centered */
    anim_to(s_left_eye,  anim_height, lv_obj_get_height(s_left_eye),  e->l_eye_h, dur);
    anim_to(s_left_eye,  anim_y,      lv_obj_get_y(s_left_eye),       L_EYE_CY - e->l_eye_h/2, dur);
    anim_to(s_right_eye, anim_height, lv_obj_get_height(s_right_eye), e->r_eye_h, dur);
    anim_to(s_right_eye, anim_y,      lv_obj_get_y(s_right_eye),      R_EYE_CY - e->r_eye_h/2, dur);

    /* Pupil size */
    anim_to(s_left_pupil,  anim_width,  lv_obj_get_width(s_left_pupil),  e->pupil_w, dur);
    anim_to(s_left_pupil,  anim_height, lv_obj_get_height(s_left_pupil), e->pupil_h, dur);
    anim_to(s_right_pupil, anim_width,  lv_obj_get_width(s_right_pupil), e->pupil_w, dur);
    anim_to(s_right_pupil, anim_height, lv_obj_get_height(s_right_pupil),e->pupil_h, dur);

    /* Pupil positions (centered in eye + offset) */
    int32_t lpx = (EYE_W - e->pupil_w)/2 + e->l_pdx;
    int32_t lpy = (e->l_eye_h - e->pupil_h)/2 + e->l_pdy;
    int32_t rpx = (EYE_W - e->pupil_w)/2 + e->r_pdx;
    int32_t rpy = (e->r_eye_h - e->pupil_h)/2 + e->r_pdy;
    anim_to(s_left_pupil,  anim_x, lv_obj_get_x(s_left_pupil),  lpx, dur);
    anim_to(s_left_pupil,  anim_y, lv_obj_get_y(s_left_pupil),  lpy, dur);
    anim_to(s_right_pupil, anim_x, lv_obj_get_x(s_right_pupil), rpx, dur);
    anim_to(s_right_pupil, anim_y, lv_obj_get_y(s_right_pupil), rpy, dur);

    /* Mouth */
    int32_t mx = (SCR_W - e->mouth_w) / 2;
    anim_to(s_mouth, anim_width,  lv_obj_get_width(s_mouth),  e->mouth_w, dur);
    anim_to(s_mouth, anim_height, lv_obj_get_height(s_mouth), e->mouth_h, dur);
    anim_to(s_mouth, anim_x,     lv_obj_get_x(s_mouth),      mx, dur);
    anim_to(s_mouth, anim_y,     lv_obj_get_y(s_mouth),      e->mouth_y, dur);
    lv_obj_set_style_bg_color(s_mouth, lv_color_hex(e->mouth_col), 0);

    /* Cheeks */
    anim_to(s_left_cheek,  anim_bg_opa, lv_obj_get_style_bg_opa(s_left_cheek, 0),  e->cheek_opa, dur);
    anim_to(s_right_cheek, anim_bg_opa, lv_obj_get_style_bg_opa(s_right_cheek, 0), e->cheek_opa, dur);

    /* Timers */
    if (s_blink_timer) { if (e->do_blink) lv_timer_resume(s_blink_timer); else lv_timer_pause(s_blink_timer); }
    if (s_look_timer)  { if (e->do_look)  lv_timer_resume(s_look_timer);  else lv_timer_pause(s_look_timer);  }

    s_emo_idx = idx;
}

/* ── Build face scene ── */
static void create_face(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_outline_width(scr, 0, 0);
    lv_obj_set_style_shadow_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_face = lv_obj_create(scr);
    lv_obj_remove_flag(s_face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s_face, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(s_face, SCR_W, SCR_H);
    lv_obj_set_pos(s_face, 0, 0);
    lv_obj_set_style_bg_opa(s_face, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_face, 0, 0);
    lv_obj_set_style_outline_width(s_face, 0, 0);
    lv_obj_set_style_shadow_width(s_face, 0, 0);
    lv_obj_set_style_pad_all(s_face, 0, 0);

    const emo_preset_t *e = &EMOTIONS[0];

    /* Eyes */
    s_left_eye  = make_box(s_face, L_EYE_X, L_EYE_Y, EYE_W, e->l_eye_h, COL_EYE, LV_OPA_COVER);
    s_right_eye = make_box(s_face, R_EYE_X, R_EYE_Y, EYE_W, e->r_eye_h, COL_EYE, LV_OPA_COVER);

    /* Pupils */
    int32_t pcx = (EYE_W - e->pupil_w) / 2;
    int32_t pcy = (e->l_eye_h - e->pupil_h) / 2;
    s_left_pupil  = make_box(s_left_eye,  pcx, pcy, e->pupil_w, e->pupil_h, COL_PUPIL, LV_OPA_COVER);
    s_right_pupil = make_box(s_right_eye, pcx, pcy, e->pupil_w, e->pupil_h, COL_PUPIL, LV_OPA_COVER);

    /* Highlights */
    make_box(s_left_pupil,  e->pupil_w - HL_SZ - 2, 2, HL_SZ, HL_SZ, COL_HL, LV_OPA_COVER);
    make_box(s_right_pupil, e->pupil_w - HL_SZ - 2, 2, HL_SZ, HL_SZ, COL_HL, LV_OPA_COVER);

    /* Cheeks */
    s_left_cheek  = make_box(s_face, L_CHEEK_X, L_CHEEK_Y, CHEEK_W, CHEEK_H, COL_CHEEK, e->cheek_opa);
    s_right_cheek = make_box(s_face, R_CHEEK_X, R_CHEEK_Y, CHEEK_W, CHEEK_H, COL_CHEEK, e->cheek_opa);

    /* Mouth */
    int32_t mx = (SCR_W - e->mouth_w) / 2;
    s_mouth = make_box(s_face, mx, e->mouth_y, e->mouth_w, e->mouth_h,
                        lv_color_hex(e->mouth_col), LV_OPA_COVER);
}

/* ── Breathing ── */
static void start_breathing(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_face);
    lv_anim_set_values(&a, 0, 4);
    lv_anim_set_exec_cb(&a, anim_tr_y);
    lv_anim_set_duration(&a, 2500);
    lv_anim_set_playback_duration(&a, 2500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ══════════════ Public API ══════════════ */

esp_err_t robot_face_start(void)
{
    if (s_active) return ESP_OK;
    if (!lvgl_port_lock(500)) { ESP_LOGE(TAG, "LVGL lock timeout"); return ESP_FAIL; }

    s_emo_idx = 0;
    create_face();
    s_blink_timer = lv_timer_create(blink_cb, 3000, NULL);
    s_look_timer  = lv_timer_create(look_cb,  4000, NULL);
    start_breathing();

    lvgl_port_unlock();
    s_active = true;
    ESP_LOGI(TAG, "Robot face started");
    return ESP_OK;
}

esp_err_t robot_face_stop(void)
{
    if (!s_active) return ESP_OK;
    if (!lvgl_port_lock(500)) { ESP_LOGE(TAG, "LVGL lock timeout"); return ESP_FAIL; }

    if (s_blink_timer) { lv_timer_delete(s_blink_timer); s_blink_timer = NULL; }
    if (s_look_timer)  { lv_timer_delete(s_look_timer);  s_look_timer  = NULL; }

    lv_anim_delete(s_face, NULL);
    lv_anim_delete(s_left_eye, NULL);    lv_anim_delete(s_right_eye, NULL);
    lv_anim_delete(s_left_pupil, NULL);  lv_anim_delete(s_right_pupil, NULL);
    lv_anim_delete(s_mouth, NULL);
    lv_anim_delete(s_left_cheek, NULL);  lv_anim_delete(s_right_cheek, NULL);

    if (s_face && lv_obj_is_valid(s_face)) lv_obj_delete(s_face);
    s_face = s_left_eye = s_right_eye = NULL;
    s_left_pupil = s_right_pupil = NULL;
    s_mouth = s_left_cheek = s_right_cheek = NULL;

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lvgl_port_unlock();

    s_active = false;
    s_emo_idx = 0;
    ESP_LOGI(TAG, "Robot face stopped");
    return ESP_OK;
}

bool robot_face_is_active(void) { return s_active; }

esp_err_t robot_face_set_emotion(const char *emotion)
{
    if (!s_active || !emotion) return ESP_ERR_INVALID_STATE;
    for (int i = 0; i < (int)EMO_COUNT; i++) {
        if (strcmp(EMOTIONS[i].name, emotion) == 0) {
            if (!lvgl_port_lock(500)) return ESP_FAIL;
            apply_emotion(i, 300);
            lvgl_port_unlock();
            ESP_LOGI(TAG, "Emotion -> %s", emotion);
            return ESP_OK;
        }
    }
    ESP_LOGW(TAG, "Unknown emotion: %s", emotion);
    return ESP_ERR_NOT_FOUND;
}

const char *robot_face_get_emotion(void)
{
    return EMOTIONS[s_emo_idx].name;
}

const char *robot_face_list_emotions(void)
{
    static char buf[600];
    buf[0] = '\0';
    for (int i = 0; i < (int)EMO_COUNT; i++) {
        if (i > 0) strcat(buf, ", ");
        strcat(buf, EMOTIONS[i].name);
        strcat(buf, " (");
        strcat(buf, EMOTIONS[i].desc);
        strcat(buf, ")");
    }
    return buf;
}
