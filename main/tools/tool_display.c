#include "tool_display.h"
#include "hal/display/lvgl_port.h"
#include "hal/hal_config.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "tool_display";

/* ── Object tracking with IDs (incremental scene management) ── */
typedef struct {
    char id[20];       /* User-provided or auto "obj_N" */
    char type[10];     /* text/rect/circle/line/arc/symbol */
    char desc[80];     /* Human-readable summary for LLM query */
    lv_obj_t *obj;
} disp_obj_t;

#define MAX_DISP_OBJS 32
static disp_obj_t s_objs[MAX_DISP_OBJS];
static int s_obj_count = 0;
static int s_id_counter = 0;
static char s_bg_color[16] = "black";

static int track_obj(lv_obj_t *obj, const char *id, const char *type, const char *desc) {
    if (s_obj_count >= MAX_DISP_OBJS) return -1;
    disp_obj_t *d = &s_objs[s_obj_count];
    if (id && id[0])
        snprintf(d->id, sizeof(d->id), "%s", id);
    else
        snprintf(d->id, sizeof(d->id), "obj_%d", ++s_id_counter);
    snprintf(d->type, sizeof(d->type), "%s", type ? type : "?");
    snprintf(d->desc, sizeof(d->desc), "%s", desc ? desc : "");
    d->obj = obj;
    return s_obj_count++;
}

static disp_obj_t *find_by_id(const char *id) {
    if (!id) return NULL;
    for (int i = 0; i < s_obj_count; i++)
        if (strcmp(s_objs[i].id, id) == 0) return &s_objs[i];
    return NULL;
}

static bool remove_by_id(const char *id) {
    for (int i = 0; i < s_obj_count; i++) {
        if (strcmp(s_objs[i].id, id) == 0) {
            if (s_objs[i].obj && lv_obj_is_valid(s_objs[i].obj))
                lv_obj_delete(s_objs[i].obj);
            for (int j = i; j < s_obj_count - 1; j++)
                s_objs[j] = s_objs[j + 1];
            s_obj_count--;
            return true;
        }
    }
    return false;
}

static void clear_all(void) {
    for (int i = 0; i < s_obj_count; i++) {
        if (s_objs[i].obj && lv_obj_is_valid(s_objs[i].obj))
            lv_obj_delete(s_objs[i].obj);
    }
    s_obj_count = 0;
    s_id_counter = 0;
}

/* ── JSON helpers ── */
static int jint(cJSON *r, const char *k, int d) {
    cJSON *i = cJSON_GetObjectItem(r, k);
    return (i && cJSON_IsNumber(i)) ? i->valueint : d;
}
static bool jbool(cJSON *r, const char *k, bool d) {
    cJSON *i = cJSON_GetObjectItem(r, k);
    return i ? cJSON_IsTrue(i) : d;
}
static const char *jstr(cJSON *r, const char *k, const char *d) {
    cJSON *i = cJSON_GetObjectItem(r, k);
    return (i && cJSON_IsString(i)) ? i->valuestring : d;
}

/* ── Color parser (supports names + hex #RRGGBB) ── */
static lv_color_t parse_color(const char *name) {
    if (!name) return lv_color_white();
    if (name[0] == '#' && strlen(name) == 7) {
        unsigned int r, g, b;
        if (sscanf(name + 1, "%02x%02x%02x", &r, &g, &b) == 3)
            return lv_color_make(r, g, b);
    }
    static const struct { const char *n; uint8_t r, g, b; } map[] = {
        {"black",0,0,0}, {"red",255,0,0}, {"green",0,200,0},
        {"blue",0,100,255}, {"yellow",255,255,0}, {"cyan",0,255,255},
        {"magenta",255,0,255}, {"gray",128,128,128}, {"orange",255,165,0},
        {"pink",255,192,203}, {"purple",128,0,128}, {"brown",139,69,19},
    };
    for (size_t i = 0; i < sizeof(map)/sizeof(map[0]); i++) {
        if (strcmp(name, map[i].n) == 0)
            return lv_color_make(map[i].r, map[i].g, map[i].b);
    }
    return lv_color_white();
}

/* ── Font selector ── */
static const lv_font_t *pick_font(int size) {
#if LV_FONT_MONTSERRAT_36
    if (size >= 32) return &lv_font_montserrat_36;
#endif
#if LV_FONT_MONTSERRAT_28
    if (size >= 25) return &lv_font_montserrat_28;
#endif
#if LV_FONT_MONTSERRAT_20
    if (size >= 17) return &lv_font_montserrat_20;
#endif
    return &lv_font_montserrat_14;
}

/* ── Alignment parser (natural language friendly) ── */
static lv_align_t parse_align(const char *a) {
    if (!a) return LV_ALIGN_TOP_LEFT;
    if (strstr(a, "center") && !strstr(a, "top") && !strstr(a, "bottom")
        && !strstr(a, "left") && !strstr(a, "right"))
        return LV_ALIGN_CENTER;
    if (strstr(a, "top") && strstr(a, "left"))     return LV_ALIGN_TOP_LEFT;
    if (strstr(a, "top") && strstr(a, "right"))    return LV_ALIGN_TOP_RIGHT;
    if (strstr(a, "bottom") && strstr(a, "left"))  return LV_ALIGN_BOTTOM_LEFT;
    if (strstr(a, "bottom") && strstr(a, "right")) return LV_ALIGN_BOTTOM_RIGHT;
    if (strstr(a, "top"))    return LV_ALIGN_TOP_MID;
    if (strstr(a, "bottom")) return LV_ALIGN_BOTTOM_MID;
    if (strstr(a, "left"))   return LV_ALIGN_LEFT_MID;
    if (strstr(a, "right"))  return LV_ALIGN_RIGHT_MID;
    return LV_ALIGN_CENTER;
}

/*
 * Circular display safe margins.
 * The 412x412 screen is circular — content at edges/corners is clipped.
 * Edge safe margin: ~40px from each side; corner needs ~65px.
 */
#define EDGE_MARGIN  40
#define CORNER_MARGIN 65

/* ── Position helper ──
 * center_offset: if true, x,y means the CENTER of the object (for shapes).
 *                The object's LVGL size must already be set before calling.
 *                Automatically adjusts so LVGL's top-left origin matches.
 */
static void pos_obj(lv_obj_t *obj, cJSON *root, bool center_offset) {
    cJSON *xi = cJSON_GetObjectItem(root, "x");
    cJSON *yi = cJSON_GetObjectItem(root, "y");
    const char *align = jstr(root, "align", NULL);

    if (xi && yi && cJSON_IsNumber(xi) && cJSON_IsNumber(yi)) {
        int px = xi->valueint;
        int py = yi->valueint;
        if (center_offset) {
            /* x,y = center of shape → offset by half size for LVGL top-left */
            px -= lv_obj_get_width(obj) / 2;
            py -= lv_obj_get_height(obj) / 2;
        }
        lv_obj_set_pos(obj, px, py);
    } else if (align) {
        int xo = jint(root, "x_offset", 0);
        int yo = jint(root, "y_offset", 0);
        lv_align_t a = parse_align(align);
        /* Auto-add safe margin so content stays inside the visible circle */
        bool at_left   = (a == LV_ALIGN_TOP_LEFT || a == LV_ALIGN_LEFT_MID || a == LV_ALIGN_BOTTOM_LEFT);
        bool at_right  = (a == LV_ALIGN_TOP_RIGHT || a == LV_ALIGN_RIGHT_MID || a == LV_ALIGN_BOTTOM_RIGHT);
        bool at_top    = (a == LV_ALIGN_TOP_LEFT || a == LV_ALIGN_TOP_MID || a == LV_ALIGN_TOP_RIGHT);
        bool at_bottom = (a == LV_ALIGN_BOTTOM_LEFT || a == LV_ALIGN_BOTTOM_MID || a == LV_ALIGN_BOTTOM_RIGHT);
        bool is_corner = (at_left || at_right) && (at_top || at_bottom);
        int m = is_corner ? CORNER_MARGIN : EDGE_MARGIN;
        if (at_left)   xo += m;
        if (at_right)  xo -= m;
        if (at_top)    yo += m;
        if (at_bottom) yo -= m;
        lv_obj_align(obj, a, xo, yo);
    } else {
        lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    }
}

/* ── LVGL symbol lookup ── */
static const char *find_symbol(const char *name) {
    if (!name) return NULL;
    static const struct { const char *k; const char *v; } syms[] = {
        {"play",LV_SYMBOL_PLAY}, {"pause",LV_SYMBOL_PAUSE},
        {"stop",LV_SYMBOL_STOP}, {"next",LV_SYMBOL_NEXT},
        {"prev",LV_SYMBOL_PREV}, {"eject",LV_SYMBOL_EJECT},
        {"volume_max",LV_SYMBOL_VOLUME_MAX}, {"volume_mid",LV_SYMBOL_VOLUME_MID},
        {"mute",LV_SYMBOL_MUTE}, {"wifi",LV_SYMBOL_WIFI},
        {"bluetooth",LV_SYMBOL_BLUETOOTH},
        {"battery_full",LV_SYMBOL_BATTERY_FULL},
        {"battery_3",LV_SYMBOL_BATTERY_3},
        {"battery_2",LV_SYMBOL_BATTERY_2},
        {"battery_1",LV_SYMBOL_BATTERY_1},
        {"battery_empty",LV_SYMBOL_BATTERY_EMPTY},
        {"charge",LV_SYMBOL_CHARGE}, {"usb",LV_SYMBOL_USB},
        {"home",LV_SYMBOL_HOME}, {"settings",LV_SYMBOL_SETTINGS},
        {"power",LV_SYMBOL_POWER}, {"refresh",LV_SYMBOL_REFRESH},
        {"ok",LV_SYMBOL_OK}, {"close",LV_SYMBOL_CLOSE},
        {"warning",LV_SYMBOL_WARNING}, {"bell",LV_SYMBOL_BELL},
        {"mail",LV_SYMBOL_ENVELOPE}, {"call",LV_SYMBOL_CALL},
        {"edit",LV_SYMBOL_EDIT}, {"trash",LV_SYMBOL_TRASH},
        {"copy",LV_SYMBOL_COPY}, {"cut",LV_SYMBOL_CUT},
        {"paste",LV_SYMBOL_PASTE}, {"save",LV_SYMBOL_SAVE},
        {"bars",LV_SYMBOL_BARS}, {"loop",LV_SYMBOL_LOOP},
        {"up",LV_SYMBOL_UP}, {"down",LV_SYMBOL_DOWN},
        {"left",LV_SYMBOL_LEFT}, {"right",LV_SYMBOL_RIGHT},
        {"plus",LV_SYMBOL_PLUS}, {"minus",LV_SYMBOL_MINUS},
        {"eye_open",LV_SYMBOL_EYE_OPEN}, {"eye_close",LV_SYMBOL_EYE_CLOSE},
        {"list",LV_SYMBOL_LIST}, {"folder",LV_SYMBOL_DIRECTORY},
        {"file",LV_SYMBOL_FILE}, {"image",LV_SYMBOL_IMAGE},
        {"audio",LV_SYMBOL_AUDIO}, {"video",LV_SYMBOL_VIDEO},
        {"download",LV_SYMBOL_DOWNLOAD}, {"upload",LV_SYMBOL_UPLOAD},
        {"gps",LV_SYMBOL_GPS}, {"keyboard",LV_SYMBOL_KEYBOARD},
        {"sd_card",LV_SYMBOL_SD_CARD}, {"drive",LV_SYMBOL_DRIVE},
        {"tint",LV_SYMBOL_TINT}, {"backspace",LV_SYMBOL_BACKSPACE},
        {"shuffle",LV_SYMBOL_SHUFFLE},
    };
    for (size_t i = 0; i < sizeof(syms)/sizeof(syms[0]); i++) {
        if (strcmp(name, syms[i].k) == 0) return syms[i].v;
    }
    return NULL;
}

/* Free heap-allocated line points on object delete */
static void line_delete_cb(lv_event_t *e) {
    void *pts = lv_obj_get_user_data(lv_event_get_target(e));
    free(pts);
}

/* ── Animation callback wrappers ── */
static void anim_set_x(void *obj, int32_t v)   { lv_obj_set_x(obj, v); }
static void anim_set_y(void *obj, int32_t v)   { lv_obj_set_y(obj, v); }
static void anim_set_opa(void *obj, int32_t v)  { lv_obj_set_style_opa(obj, v, 0); }
static void anim_set_w(void *obj, int32_t v)    { lv_obj_set_width(obj, v); }
static void anim_set_h(void *obj, int32_t v)    { lv_obj_set_height(obj, v); }

/* ══════════════════════════════════════════════════
 *  Main execute — each call adds an element to screen
 * ══════════════════════════════════════════════════ */
esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json ? input_json : "{}");
    if (!root) {
        snprintf(output, output_size, "Error: invalid JSON");
        return ESP_FAIL;
    }

    const char *action = jstr(root, "action", "text");
    ESP_LOGI(TAG, "action=%s", action);

    if (!lvgl_port_lock(200)) {
        cJSON_Delete(root);
        snprintf(output, output_size, "Error: LVGL lock timeout");
        return ESP_FAIL;
    }

    lv_obj_t *scr = lv_screen_active();

    const char *oid = jstr(root, "id", NULL);

    /* ── clear ── */
    if (strcmp(action, "clear") == 0) {
        clear_all();
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        snprintf(s_bg_color, sizeof(s_bg_color), "black");
        snprintf(output, output_size, "Screen cleared, 0 objects");

    /* ── fill ── */
    } else if (strcmp(action, "fill") == 0) {
        const char *fc = jstr(root, "color", "black");
        lv_obj_set_style_bg_color(scr, parse_color(fc), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        snprintf(s_bg_color, sizeof(s_bg_color), "%.15s", fc);
        snprintf(output, output_size, "Background=%s, %d objects", fc, s_obj_count);

    /* ── text ── */
    } else if (strcmp(action, "text") == 0) {
        const char *text = jstr(root, "text", "");
        if (text[0] == '\0') {
            snprintf(output, output_size, "Error: text required");
        } else {
            lv_obj_t *lbl = lv_label_create(scr);
            int mw = jint(root, "max_width", 0);
            if (mw > 0) {
                /* Explicit max_width → wrap mode for long text */
                lv_obj_set_width(lbl, mw);
                lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
            } else {
                /* Auto-size so alignment actually moves the text */
                lv_obj_set_width(lbl, LV_SIZE_CONTENT);
            }
            lv_obj_set_style_text_color(lbl, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_text_font(lbl, pick_font(jint(root, "font_size", 14)), 0);
            const char *bg = jstr(root, "bg_color", NULL);
            if (bg) {
                lv_obj_set_style_bg_color(lbl, parse_color(bg), 0);
                lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
                lv_obj_set_style_pad_all(lbl, 4, 0);
            }
            lv_label_set_text(lbl, text);
            pos_obj(lbl, root, false);
            char desc[80];
            snprintf(desc, sizeof(desc), "'%.30s' %s font=%d",
                     text, jstr(root, "color", "white"), jint(root, "font_size", 14));
            track_obj(lbl, oid, "text", desc);
            snprintf(output, output_size, "[%s] text at(%d,%d), %d objs",
                     s_objs[s_obj_count-1].id,
                     (int)lv_obj_get_x(lbl), (int)lv_obj_get_y(lbl), s_obj_count);
        }

    /* ── rect ── */
    } else if (strcmp(action, "rect") == 0) {
        int w = jint(root, "width", 50), h = jint(root, "height", 50);
        lv_obj_t *obj = lv_obj_create(scr);
        lv_obj_remove_style_all(obj);
        lv_obj_set_size(obj, w, h);
        lv_obj_set_style_radius(obj, jint(root, "radius", 0), 0);
        if (jbool(root, "fill", true)) {
            lv_obj_set_style_bg_color(obj, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(obj, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_border_width(obj, jint(root, "border_width", 2), 0);
        }
        pos_obj(obj, root, true);
        int cx = lv_obj_get_x(obj) + w/2, cy = lv_obj_get_y(obj) + h/2;
        char desc[80];
        snprintf(desc, sizeof(desc), "%dx%d center(%d,%d) %s%s", w, h, cx, cy,
                 jstr(root, "color", "white"), jbool(root, "fill", true) ? "" : " outline");
        track_obj(obj, oid, "rect", desc);
        snprintf(output, output_size, "[%s] rect %dx%d center(%d,%d), %d objs",
                 s_objs[s_obj_count-1].id, w, h, cx, cy, s_obj_count);

    /* ── circle ── */
    } else if (strcmp(action, "circle") == 0) {
        int r = jint(root, "radius", 25);
        lv_obj_t *obj = lv_obj_create(scr);
        lv_obj_remove_style_all(obj);
        lv_obj_set_size(obj, r * 2, r * 2);
        lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
        if (jbool(root, "fill", true)) {
            lv_obj_set_style_bg_color(obj, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(obj, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_border_width(obj, jint(root, "border_width", 2), 0);
        }
        pos_obj(obj, root, true);
        int cx = lv_obj_get_x(obj) + r, cy = lv_obj_get_y(obj) + r;
        char desc[80];
        snprintf(desc, sizeof(desc), "r=%d center(%d,%d) %s%s", r, cx, cy,
                 jstr(root, "color", "white"), jbool(root, "fill", true) ? "" : " outline");
        track_obj(obj, oid, "circle", desc);
        snprintf(output, output_size, "[%s] circle r=%d center(%d,%d), %d objs",
                 s_objs[s_obj_count-1].id, r, cx, cy, s_obj_count);

    /* ── line ── */
    } else if (strcmp(action, "line") == 0) {
        lv_point_precise_t *pts = malloc(2 * sizeof(lv_point_precise_t));
        if (!pts) {
            snprintf(output, output_size, "Error: alloc failed");
        } else {
            pts[0].x = jint(root, "x", 0);  pts[0].y = jint(root, "y", 0);
            pts[1].x = jint(root, "x2", 100); pts[1].y = jint(root, "y2", 100);
            lv_obj_t *ln = lv_line_create(scr);
            lv_line_set_points(ln, pts, 2);
            lv_obj_set_style_line_color(ln, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_line_width(ln, jint(root, "width", 2), 0);
            lv_obj_set_user_data(ln, pts);
            lv_obj_add_event_cb(ln, line_delete_cb, LV_EVENT_DELETE, NULL);
            char desc[80];
            snprintf(desc, sizeof(desc), "(%d,%d)->(%d,%d) %s w=%d",
                     (int)pts[0].x, (int)pts[0].y, (int)pts[1].x, (int)pts[1].y,
                     jstr(root, "color", "white"), jint(root, "width", 2));
            track_obj(ln, oid, "line", desc);
            snprintf(output, output_size, "[%s] line (%d,%d)->(%d,%d), %d objs",
                     s_objs[s_obj_count-1].id,
                     (int)pts[0].x, (int)pts[0].y, (int)pts[1].x, (int)pts[1].y, s_obj_count);
        }

    /* ── arc ── */
    } else if (strcmp(action, "arc") == 0) {
        int r = jint(root, "radius", 50);
        int sa = jint(root, "start_angle", 0);
        int ea = jint(root, "end_angle", 270);
        lv_obj_t *arc = lv_arc_create(scr);
        lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        lv_obj_set_size(arc, r * 2, r * 2);
        lv_arc_set_bg_angles(arc, sa, ea);
        lv_obj_set_style_arc_color(arc, parse_color(jstr(root, "color", "white")), LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(arc, jint(root, "width", 3), LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
        pos_obj(arc, root, true);
        int cx = lv_obj_get_x(arc) + r, cy = lv_obj_get_y(arc) + r;
        char desc[80];
        snprintf(desc, sizeof(desc), "%d-%ddeg r=%d center(%d,%d) %s", sa, ea, r, cx, cy,
                 jstr(root, "color", "white"));
        track_obj(arc, oid, "arc", desc);
        snprintf(output, output_size, "[%s] arc %d-%ddeg r=%d center(%d,%d), %d objs",
                 s_objs[s_obj_count-1].id, sa, ea, r, cx, cy, s_obj_count);

    /* ── symbol (built-in icons) ── */
    } else if (strcmp(action, "symbol") == 0) {
        const char *name = jstr(root, "name", "");
        const char *sym = find_symbol(name);
        if (!sym) {
            snprintf(output, output_size,
                "Unknown symbol '%s'. Available: ok, close, warning, home, settings, "
                "wifi, bluetooth, battery_full, charge, play, pause, stop, bell, mail, "
                "power, refresh, edit, trash, plus, minus, up, down, left, right, "
                "eye_open, eye_close, folder, file, image, audio, video, download, upload",
                name);
        } else {
            lv_obj_t *lbl = lv_label_create(scr);
            lv_obj_set_style_text_color(lbl, parse_color(jstr(root, "color", "white")), 0);
            lv_obj_set_style_text_font(lbl, pick_font(jint(root, "font_size", 28)), 0);
            lv_label_set_text(lbl, sym);
            pos_obj(lbl, root, false);
            char desc[80];
            snprintf(desc, sizeof(desc), "%s %s font=%d", name,
                     jstr(root, "color", "white"), jint(root, "font_size", 28));
            track_obj(lbl, oid, "symbol", desc);
            snprintf(output, output_size, "[%s] symbol '%s', %d objs",
                     s_objs[s_obj_count-1].id, name, s_obj_count);
        }

    /* ── query — report current scene state to LLM ── */
    } else if (strcmp(action, "query") == 0) {
        int pos = 0;
        pos += snprintf(output + pos, output_size - pos,
            "Screen 412x412 circle, bg=%s, %d objects:\n", s_bg_color, s_obj_count);
        for (int i = 0; i < s_obj_count && pos < (int)output_size - 100; i++) {
            pos += snprintf(output + pos, output_size - pos,
                "[%s] %s: %s\n", s_objs[i].id, s_objs[i].type, s_objs[i].desc);
        }

    /* ── delete — remove a specific object by id ── */
    } else if (strcmp(action, "delete") == 0) {
        if (!oid || !oid[0]) {
            snprintf(output, output_size, "Error: 'id' required for delete");
        } else if (remove_by_id(oid)) {
            snprintf(output, output_size, "Deleted '%s', %d objects remain", oid, s_obj_count);
        } else {
            snprintf(output, output_size, "Error: id '%s' not found", oid);
        }

    /* ── update — modify an existing object by id ── */
    } else if (strcmp(action, "update") == 0) {
        if (!oid || !oid[0]) {
            snprintf(output, output_size, "Error: 'id' required for update");
        } else {
            disp_obj_t *d = find_by_id(oid);
            if (!d || !d->obj || !lv_obj_is_valid(d->obj)) {
                snprintf(output, output_size, "Error: id '%s' not found", oid);
            } else {
                /* Update text content */
                const char *new_text = jstr(root, "text", NULL);
                if (new_text && (strcmp(d->type, "text") == 0 || strcmp(d->type, "symbol") == 0))
                    lv_label_set_text(d->obj, new_text);
                /* Update color */
                const char *nc = jstr(root, "color", NULL);
                if (nc) {
                    lv_color_t c = parse_color(nc);
                    if (strcmp(d->type, "text") == 0 || strcmp(d->type, "symbol") == 0)
                        lv_obj_set_style_text_color(d->obj, c, 0);
                    else if (strcmp(d->type, "arc") == 0)
                        lv_obj_set_style_arc_color(d->obj, c, LV_PART_INDICATOR);
                    else if (strcmp(d->type, "line") == 0)
                        lv_obj_set_style_line_color(d->obj, c, 0);
                    else
                        lv_obj_set_style_bg_color(d->obj, c, 0);
                }
                /* Update position */
                cJSON *xi = cJSON_GetObjectItem(root, "x");
                cJSON *yi = cJSON_GetObjectItem(root, "y");
                if (xi && yi && cJSON_IsNumber(xi) && cJSON_IsNumber(yi)) {
                    bool is_shape = strcmp(d->type, "rect") == 0
                                 || strcmp(d->type, "circle") == 0
                                 || strcmp(d->type, "arc") == 0;
                    int px = xi->valueint, py = yi->valueint;
                    if (is_shape) {
                        px -= lv_obj_get_width(d->obj) / 2;
                        py -= lv_obj_get_height(d->obj) / 2;
                    }
                    lv_obj_set_pos(d->obj, px, py);
                }
                /* Update opacity (0=invisible, 255=opaque) */
                int opa = jint(root, "opacity", -1);
                if (opa >= 0)
                    lv_obj_set_style_opa(d->obj, opa > 255 ? 255 : opa, 0);
                /* Update size */
                int nw = jint(root, "width", -1);
                int nh = jint(root, "height", -1);
                if (nw > 0) lv_obj_set_width(d->obj, nw);
                if (nh > 0) lv_obj_set_height(d->obj, nh);
                snprintf(output, output_size, "Updated '%s' (%s)", oid, d->type);
            }
        }

    /* ── animate — animate a property of an existing object ── */
    } else if (strcmp(action, "animate") == 0) {
        if (!oid || !oid[0]) {
            snprintf(output, output_size, "Error: 'id' required for animate");
        } else {
            disp_obj_t *d = find_by_id(oid);
            if (!d || !d->obj || !lv_obj_is_valid(d->obj)) {
                snprintf(output, output_size, "Error: id '%s' not found", oid);
            } else {
                const char *prop = jstr(root, "property", "x");
                int from_val = jint(root, "from", 0);
                int to_val = jint(root, "to", 100);
                int dur = jint(root, "duration", 500);
                int delay_ms = jint(root, "delay", 0);
                int repeat = jint(root, "repeat", 0);
                bool playback = jbool(root, "playback", false);
                const char *path = jstr(root, "path", "ease_in_out");

                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, d->obj);
                lv_anim_set_values(&a, from_val, to_val);
                lv_anim_set_duration(&a, dur);
                if (delay_ms > 0) lv_anim_set_delay(&a, delay_ms);

                /* Select property */
                if (strcmp(prop, "y") == 0)
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_set_y);
                else if (strcmp(prop, "opacity") == 0)
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_set_opa);
                else if (strcmp(prop, "width") == 0)
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_set_w);
                else if (strcmp(prop, "height") == 0)
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_set_h);
                else
                    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_set_x);

                /* Select easing path */
                if (strcmp(path, "linear") == 0)
                    lv_anim_set_path_cb(&a, lv_anim_path_linear);
                else if (strcmp(path, "ease_in") == 0)
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
                else if (strcmp(path, "ease_out") == 0)
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
                else if (strcmp(path, "bounce") == 0)
                    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
                else if (strcmp(path, "overshoot") == 0)
                    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
                else
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

                /* Repeat & playback */
                if (repeat < 0)
                    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
                else if (repeat > 0)
                    lv_anim_set_repeat_count(&a, repeat);
                if (playback)
                    lv_anim_set_playback_duration(&a, dur);

                lv_anim_start(&a);
                snprintf(output, output_size, "Animating '%s' %s %d->%d in %dms%s",
                         oid, prop, from_val, to_val, dur,
                         repeat < 0 ? " (infinite)" : playback ? " (pingpong)" : "");
            }
        }

    /* ── unknown ── */
    } else {
        snprintf(output, output_size, "Unknown action '%s'. Use: text, rect, circle, "
                 "line, arc, symbol, clear, fill, query, delete, update, animate", action);
    }

    lv_refr_now(lv_display_get_default());
    lvgl_port_unlock();
    cJSON_Delete(root);
    return ESP_OK;
}
