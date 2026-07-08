#ifndef APPLICATIONS_MODULES_STORAGE_MANAGER_H_
#define APPLICATIONS_MODULES_STORAGE_MANAGER_H_

#include "etc_types.h"

rt_err_t etc_storage_init(void);

rt_err_t etc_storage_read_user(const char *uid, etc_data_t *data);

rt_err_t etc_storage_write_user(const char *uid, const etc_data_t *data);

rt_err_t etc_storage_delete_user(const char *uid);

void etc_storage_register_defaults(void);

rt_err_t etc_storage_save_offline_log(const char *uid, const etc_data_t *data);

rt_err_t etc_storage_get_oldest_offline_log(etc_log_t *log_out, char *filename_out, rt_size_t name_len);

rt_err_t etc_storage_delete_offline_log(const char *filename);

#endif
