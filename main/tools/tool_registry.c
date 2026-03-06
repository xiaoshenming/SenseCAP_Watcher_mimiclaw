#include "tool_registry.h"
#include "mimi_config.h"
#include "tools/tool_web_search.h"
#include "tools/tool_get_time.h"
#include "tools/tool_files.h"
#include "tools/tool_cron.h"
#include "tools/tool_camera.h"
#include "tools/tool_display.h"
#include "tools/tool_sysinfo.h"

#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "tools";

#define MAX_TOOLS 12

static mimi_tool_t s_tools[MAX_TOOLS];
static int s_tool_count = 0;
static char *s_tools_json = NULL;  /* cached JSON array string */

static void register_tool(const mimi_tool_t *tool)
{
    if (s_tool_count >= MAX_TOOLS) {
        ESP_LOGE(TAG, "Tool registry full");
        return;
    }
    s_tools[s_tool_count++] = *tool;
    ESP_LOGI(TAG, "Registered tool: %s", tool->name);
}

static void build_tools_json(void)
{
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < s_tool_count; i++) {
        cJSON *tool = cJSON_CreateObject();
        cJSON_AddStringToObject(tool, "name", s_tools[i].name);
        cJSON_AddStringToObject(tool, "description", s_tools[i].description);

        cJSON *schema = cJSON_Parse(s_tools[i].input_schema_json);
        if (schema) {
            cJSON_AddItemToObject(tool, "input_schema", schema);
        }

        cJSON_AddItemToArray(arr, tool);
    }

    free(s_tools_json);
    s_tools_json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    ESP_LOGI(TAG, "Tools JSON built (%d tools)", s_tool_count);
}

esp_err_t tool_registry_init(void)
{
    s_tool_count = 0;

    /* Register web_search */
    tool_web_search_init();

    mimi_tool_t ws = {
        .name = "web_search",
        .description = "Search the web for current information. Use this when you need up-to-date facts, news, weather, or anything beyond your training data.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"The search query\"}},"
            "\"required\":[\"query\"]}",
        .execute = tool_web_search_execute,
    };
    register_tool(&ws);

    /* Register get_current_time */
    mimi_tool_t gt = {
        .name = "get_current_time",
        .description = "Get the current date and time. Also sets the system clock. Call this when you need to know what time or date it is.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_get_time_execute,
    };
    register_tool(&gt);

    /* Register read_file */
    mimi_tool_t rf = {
        .name = "read_file",
        .description = "Read a file from SPIFFS storage. Path must start with " MIMI_SPIFFS_BASE "/.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"}},"
            "\"required\":[\"path\"]}",
        .execute = tool_read_file_execute,
    };
    register_tool(&rf);

    /* Register write_file */
    mimi_tool_t wf = {
        .name = "write_file",
        .description = "Write or overwrite a file on SPIFFS storage. Path must start with " MIMI_SPIFFS_BASE "/.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"},"
            "\"content\":{\"type\":\"string\",\"description\":\"File content to write\"}},"
            "\"required\":[\"path\",\"content\"]}",
        .execute = tool_write_file_execute,
    };
    register_tool(&wf);

    /* Register edit_file */
    mimi_tool_t ef = {
        .name = "edit_file",
        .description = "Find and replace text in a file on SPIFFS. Replaces first occurrence of old_string with new_string.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_SPIFFS_BASE "/\"},"
            "\"old_string\":{\"type\":\"string\",\"description\":\"Text to find\"},"
            "\"new_string\":{\"type\":\"string\",\"description\":\"Replacement text\"}},"
            "\"required\":[\"path\",\"old_string\",\"new_string\"]}",
        .execute = tool_edit_file_execute,
    };
    register_tool(&ef);

    /* Register list_dir */
    mimi_tool_t ld = {
        .name = "list_dir",
        .description = "List files on SPIFFS storage, optionally filtered by path prefix.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"prefix\":{\"type\":\"string\",\"description\":\"Optional path prefix filter, e.g. " MIMI_SPIFFS_BASE "/memory/\"}},"
            "\"required\":[]}",
        .execute = tool_list_dir_execute,
    };
    register_tool(&ld);

    /* Register cron_add */
    mimi_tool_t ca = {
        .name = "cron_add",
        .description = "Schedule a recurring or one-shot task. The message will trigger an agent turn when the job fires.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"name\":{\"type\":\"string\",\"description\":\"Short name for the job\"},"
            "\"schedule_type\":{\"type\":\"string\",\"description\":\"'every' for recurring interval or 'at' for one-shot at a unix timestamp\"},"
            "\"interval_s\":{\"type\":\"integer\",\"description\":\"Interval in seconds (required for 'every')\"},"
            "\"at_epoch\":{\"type\":\"integer\",\"description\":\"Unix timestamp to fire at (required for 'at')\"},"
            "\"message\":{\"type\":\"string\",\"description\":\"Message to inject when the job fires, triggering an agent turn\"},"
            "\"channel\":{\"type\":\"string\",\"description\":\"Optional reply channel (e.g. 'telegram'). If omitted, current turn channel is used when available\"},"
            "\"chat_id\":{\"type\":\"string\",\"description\":\"Optional reply chat_id. Required when channel='telegram'. If omitted during a Telegram turn, current chat_id is used\"}"
            "},"
            "\"required\":[\"name\",\"schedule_type\",\"message\"]}",
        .execute = tool_cron_add_execute,
    };
    register_tool(&ca);

    /* Register cron_list */
    mimi_tool_t cl = {
        .name = "cron_list",
        .description = "List all scheduled cron jobs with their status, schedule, and IDs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_cron_list_execute,
    };
    register_tool(&cl);

    /* Register cron_remove */
    mimi_tool_t cr = {
        .name = "cron_remove",
        .description = "Remove a scheduled cron job by its ID.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"job_id\":{\"type\":\"string\",\"description\":\"The 8-character job ID to remove\"}},"
            "\"required\":[\"job_id\"]}",
        .execute = tool_cron_remove_execute,
    };
    register_tool(&cr);

    /* Register camera_capture */
    mimi_tool_t cam = {
        .name = "camera_capture",
        .description = "Capture a photo using the device camera. Can optionally send the photo via Telegram.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"action\":{\"type\":\"string\",\"description\":\"Action to perform, must be 'capture'\"},"
            "\"send_telegram\":{\"type\":\"boolean\",\"description\":\"Whether to send the photo via Telegram\"},"
            "\"chat_id\":{\"type\":\"string\",\"description\":\"Telegram chat ID (required if send_telegram=true)\"}"
            "},"
            "\"required\":[\"action\"]}",
        .execute = tool_camera_execute,
    };
    register_tool(&cam);

    /* Register display — full drawing toolkit for the 412×412 circular screen */
    mimi_tool_t disp = {
        .name = "display",
        .description =
            "Draw on the device's 412x412 pixel CIRCULAR screen. IMPORTANT: the screen is ROUND — edges and corners are clipped! "
            "Edge-aligned content automatically gets safe margins so it stays visible inside the circle. "
            "ALWAYS call this tool to show something on screen — never just claim you displayed it. "
            "Each call ADDS an element; use clear first to reset. "
            "POSITIONING (choose one): "
            "1) align='center|top|bottom|left|right|top_left|top_right|bottom_left|bottom_right' — RECOMMENDED, auto-positioned inside safe area. "
            "2) x,y pixels — absolute position (be careful near edges). "
            "ACTIONS: "
            "text — display text (font_size: 14/20/28/36, use max_width for long text wrapping). "
            "rect — rectangle. circle — circle. line — line. arc — arc segment. "
            "symbol — built-in icons (ok,close,warning,home,settings,wifi,bluetooth,battery_full,charge,play,pause,stop,bell,mail,"
            "power,refresh,edit,trash,plus,minus,up,down,left,right,eye_open,eye_close,folder,file,image,audio,video). "
            "clear — remove all. fill — set background color. "
            "COLORS: black,white,red,green,blue,yellow,cyan,magenta,gray,orange,pink,purple,brown or '#RRGGBB'.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"action\":{\"type\":\"string\",\"description\":\"text|rect|circle|line|arc|symbol|clear|fill\"},"
            "\"text\":{\"type\":\"string\",\"description\":\"Text content (for text action)\"},"
            "\"name\":{\"type\":\"string\",\"description\":\"Symbol name (for symbol action)\"},"
            "\"x\":{\"type\":\"integer\",\"description\":\"X pixel (0-411) or line start X\"},"
            "\"y\":{\"type\":\"integer\",\"description\":\"Y pixel (0-411) or line start Y\"},"
            "\"x2\":{\"type\":\"integer\",\"description\":\"Line end X\"},"
            "\"y2\":{\"type\":\"integer\",\"description\":\"Line end Y\"},"
            "\"align\":{\"type\":\"string\",\"description\":\"Named position: center,top,bottom,left,right,top_left,top_right,bottom_left,bottom_right\"},"
            "\"x_offset\":{\"type\":\"integer\",\"description\":\"Pixel offset from align position\"},"
            "\"y_offset\":{\"type\":\"integer\",\"description\":\"Pixel offset from align position\"},"
            "\"width\":{\"type\":\"integer\",\"description\":\"Rect width or line thickness\"},"
            "\"height\":{\"type\":\"integer\",\"description\":\"Rect height\"},"
            "\"radius\":{\"type\":\"integer\",\"description\":\"Circle/arc radius or rect corner radius\"},"
            "\"start_angle\":{\"type\":\"integer\",\"description\":\"Arc start angle in degrees\"},"
            "\"end_angle\":{\"type\":\"integer\",\"description\":\"Arc end angle in degrees\"},"
            "\"font_size\":{\"type\":\"integer\",\"description\":\"Text size: 14(small), 20(medium), 28(large), 36(huge)\"},"
            "\"max_width\":{\"type\":\"integer\",\"description\":\"Text wrap width in pixels\"},"
            "\"color\":{\"type\":\"string\",\"description\":\"Color name or hex #RRGGBB\"},"
            "\"bg_color\":{\"type\":\"string\",\"description\":\"Text background color\"},"
            "\"fill\":{\"type\":\"boolean\",\"description\":\"Fill shape (true) or outline only (false)\"},"
            "\"border_width\":{\"type\":\"integer\",\"description\":\"Outline thickness for unfilled shapes\"}"
            "},"
            "\"required\":[\"action\"]}",
        .execute = tool_display_execute,
    };
    register_tool(&disp);

    /* Register get_system_info */
    mimi_tool_t sysinfo = {
        .name = "get_system_info",
        .description = "Get device system information like battery level, WiFi status, etc.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"info_type\":{\"type\":\"string\",\"description\":\"Type of info: 'battery', 'wifi', or 'all'\"}},"
            "\"required\":[]}",
        .execute = tool_sysinfo_execute,
    };
    register_tool(&sysinfo);

    build_tools_json();

    ESP_LOGI(TAG, "Tool registry initialized");
    return ESP_OK;
}

const char *tool_registry_get_tools_json(void)
{
    return s_tools_json;
}

esp_err_t tool_registry_execute(const char *name, const char *input_json,
                                char *output, size_t output_size)
{
    for (int i = 0; i < s_tool_count; i++) {
        if (strcmp(s_tools[i].name, name) == 0) {
            ESP_LOGI(TAG, "Executing tool: %s", name);
            return s_tools[i].execute(input_json, output, output_size);
        }
    }

    ESP_LOGW(TAG, "Unknown tool: %s", name);
    snprintf(output, output_size, "Error: unknown tool '%s'", name);
    return ESP_ERR_NOT_FOUND;
}
