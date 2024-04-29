#ifndef _SILK_H_
#define _SILK_H_

#define SILK_API __attribute__((visibility("default")))

SILK_API int silk_run_file(const char* filename);
SILK_API int silk_run_string(const char* js_data);
SILK_API int silk_run(const char* js_data, const char* js_data_end);

#endif
