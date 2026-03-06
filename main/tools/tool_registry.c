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
            "Draw on the 412x412 CIRCULAR screen. Center=(206,206). ALWAYS call this tool — never just claim you displayed it. "
            "INCREMENTAL: Each call ADDS an element. Give objects an 'id' (e.g. 'board','x1','eye_l') for later reference. "
            "DO NOT clear+redraw everything each turn! Use 'query' to see current scene, 'update'/'delete' to modify, 'animate' for motion. "
            "ACTIONS: "
            "query — returns all objects on screen (call first if unsure what's displayed). "
            "text — add text (font_size:14/20/28/36). "
            "rect — rectangle. circle — circle. line — line. arc — arc. "
            "symbol — icons (ok,close,warning,home,wifi,play,pause,bell,mail,power,etc). "
            "update — modify existing object by id: change color/position/text/opacity/size. "
            "delete — remove one object by id (not clear all). "
            "animate — animate object property: {id,property:x|y|opacity|width|height,from,to,duration,path:linear|ease_in|ease_out|ease_in_out|bounce|overshoot,repeat:-1=infinite,playback:true=pingpong}. "
            "clear — remove ALL objects. fill — set background color. "
            "COLORS: black,white,red,green,blue,yellow,cyan,magenta,gray,orange,pink,purple,brown or '#RRGGBB'. "
            "POSITIONING: align='center|top|bottom|left|right|...' (auto safe area), or x,y pixels (shapes=center, text=top-left).",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{"
            "\"action\":{\"type\":\"string\",\"description\":\"query|text|rect|circle|line|arc|symbol|update|delete|animate|clear|fill\"},"
            "\"id\":{\"type\":\"string\",\"description\":\"Object ID for tracking. Set on create; required for update/delete/animate\"},"
            "\"text\":{\"type\":\"string\",\"description\":\"Text content (text action) or new text (update action)\"},"
            "\"name\":{\"type\":\"string\",\"description\":\"Symbol name (symbol action)\"},"
            "\"x\":{\"type\":\"integer\",\"description\":\"X pixel. Shapes=center, text=top-left\"},"
            "\"y\":{\"type\":\"integer\",\"description\":\"Y pixel. Shapes=center, text=top-left\"},"
            "\"x2\":{\"type\":\"integer\",\"description\":\"Line end X\"},"
            "\"y2\":{\"type\":\"integer\",\"description\":\"Line end Y\"},"
            "\"align\":{\"type\":\"string\",\"description\":\"Named position: center,top,bottom,left,right,top_left,top_right,bottom_left,bottom_right\"},"
            "\"x_offset\":{\"type\":\"integer\",\"description\":\"Pixel offset from align\"},"
            "\"y_offset\":{\"type\":\"integer\",\"description\":\"Pixel offset from align\"},"
            "\"width\":{\"type\":\"integer\",\"description\":\"Rect width or line thickness\"},"
            "\"height\":{\"type\":\"integer\",\"description\":\"Rect height\"},"
            "\"radius\":{\"type\":\"integer\",\"description\":\"Circle/arc radius or rect corner radius\"},"
            "\"start_angle\":{\"type\":\"integer\",\"description\":\"Arc start degrees\"},"
            "\"end_angle\":{\"type\":\"integer\",\"description\":\"Arc end degrees\"},"
            "\"font_size\":{\"type\":\"integer\",\"description\":\"14(small),20(medium),28(large),36(huge)\"},"
            "\"max_width\":{\"type\":\"integer\",\"description\":\"Text wrap width\"},"
            "\"color\":{\"type\":\"string\",\"description\":\"Color name or #RRGGBB\"},"
            "\"bg_color\":{\"type\":\"string\",\"description\":\"Text background color\"},"
            "\"fill\":{\"type\":\"boolean\",\"description\":\"Fill shape (true) or outline (false)\"},"
            "\"border_width\":{\"type\":\"integer\",\"description\":\"Outline thickness\"},"
            "\"opacity\":{\"type\":\"integer\",\"description\":\"0-255 for update action\"},"
            "\"property\":{\"type\":\"string\",\"description\":\"Animate: x|y|opacity|width|height\"},"
            "\"from\":{\"type\":\"integer\",\"description\":\"Animation start value\"},"
            "\"to\":{\"type\":\"integer\",\"description\":\"Animation end value\"},"
            "\"duration\":{\"type\":\"integer\",\"description\":\"Animation ms (default 500)\"},"
            "\"delay\":{\"type\":\"integer\",\"description\":\"Animation delay ms\"},"
            "\"repeat\":{\"type\":\"integer\",\"description\":\"Animation repeat: 0=once,-1=infinite,N=count\"},"
            "\"playback\":{\"type\":\"boolean\",\"description\":\"Animate back and forth (pingpong)\"},"
            "\"path\":{\"type\":\"string\",\"description\":\"Easing: linear|ease_in|ease_out|ease_in_out|bounce|overshoot\"}"
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
