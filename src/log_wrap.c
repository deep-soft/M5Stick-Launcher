#include "esp_log.h"
#include <stdarg.h>

void __wrap_esp_log_writev(esp_log_level_t level, const char *tag, const char *format, va_list args) {
    esp_log_writev(level, tag, format, args);
}

void __wrap_esp_log_write(esp_log_level_t level, const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);
    __wrap_esp_log_writev(level, tag, format, args);
    va_end(args);
}
