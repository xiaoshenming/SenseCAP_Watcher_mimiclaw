#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t robot_face_start(void);
esp_err_t robot_face_stop(void);
bool robot_face_is_active(void);

esp_err_t robot_face_set_emotion(const char *emotion);
const char *robot_face_get_emotion(void);
const char *robot_face_list_emotions(void);

#ifdef __cplusplus
}
#endif
