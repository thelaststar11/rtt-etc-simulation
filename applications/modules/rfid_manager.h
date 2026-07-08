#ifndef APPLICATIONS_MODULES_RFID_MANAGER_H_
#define APPLICATIONS_MODULES_RFID_MANAGER_H_

#include <rtthread.h>

rt_err_t rfid_manager_init(void);

rt_err_t rfid_manager_read_uid(char *uid_buf, rt_size_t buf_len);

#endif /* APPLICATIONS_MODULES_RFID_MANAGER_H_ */
