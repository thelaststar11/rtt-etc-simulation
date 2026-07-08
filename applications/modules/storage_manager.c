#include "storage_manager.h"
#include <rtthread.h>
#include <string.h>

#define DB_DIR_PATH         "/sdcard/etc_users"
#define LOG_DIR_PATH        "/sdcard/etc_logs"
#define MAX_PATH_LEN        64
#define RAM_DB_MAX_USERS    10

static struct rt_mutex storage_mutex;

#ifdef RT_USING_DFS
#include <dfs_posix.h>

static void get_user_file_path(const char *uid, char *path_buf, rt_size_t buf_len)
{
    rt_snprintf(path_buf, buf_len, "%s/%s.bin", DB_DIR_PATH, uid);
}
#else
typedef struct {
    char uid[16];
    etc_data_t etc_data;
    rt_bool_t is_valid;
} ram_user_t;

static ram_user_t ram_user_db[RAM_DB_MAX_USERS];
#endif

rt_err_t etc_storage_init(void)
{
    if (rt_mutex_init(&storage_mutex, "store_mtx", RT_IPC_FLAG_PRIO) != RT_EOK)
    {
        rt_kprintf("[Storage] Error: Failed to init mutex\n");
        return -RT_ERROR;
    }

#ifdef RT_USING_DFS
    /* 检查并创建基础目录 */
    DIR *dir = opendir(DB_DIR_PATH);
    if (dir == RT_NULL)
    {
        mkdir(DB_DIR_PATH, 0777);
    }
    else
    {
        closedir(dir);
    }

    /* 检查并创建离线日志日志目录 */
    DIR *log_dir = opendir(LOG_DIR_PATH);
    if (log_dir == RT_NULL)
    {
        mkdir(LOG_DIR_PATH, 0777);
    }
    else
    {
        closedir(log_dir);
    }
#else
    rt_memset(ram_user_db, 0, sizeof(ram_user_db));
    rt_kprintf("[Storage] Notice: DFS not enabled, using RAM fallback.\n");
#endif

    return RT_EOK;
}

rt_err_t etc_storage_read_user(const char *uid, etc_data_t *data)
{
    rt_err_t result = -RT_ERROR;
    if (uid == RT_NULL || data == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    char file_path[MAX_PATH_LEN];
    get_user_file_path(uid, file_path, sizeof(file_path));

    int fd = open(file_path, O_RDONLY);
    if (fd >= 0)
    {
        int bytes = read(fd, data, sizeof(etc_data_t));
        close(fd);
        if (bytes == sizeof(etc_data_t))
        {
            result = RT_EOK;
        }
    }
#else
    for (int i = 0; i < RAM_DB_MAX_USERS; i++)
    {
        if (ram_user_db[i].is_valid && rt_strcmp(ram_user_db[i].uid, uid) == 0)
        {
            rt_memcpy(data, &ram_user_db[i].etc_data, sizeof(etc_data_t));
            result = RT_EOK;
            break;
        }
    }
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

rt_err_t etc_storage_write_user(const char *uid, const etc_data_t *data)
{
    rt_err_t result = -RT_ERROR;
    if (uid == RT_NULL || data == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    char file_path[MAX_PATH_LEN];
    char tmp_path[MAX_PATH_LEN];
    get_user_file_path(uid, file_path, sizeof(file_path));
    rt_snprintf(tmp_path, sizeof(tmp_path), "%s/%s.tmp", DB_DIR_PATH, uid);

    /* 1. 写入临时文件 (不直接改写原文件) */
    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0)
    {
        int bytes = write(fd, data, sizeof(etc_data_t));
        close(fd);

        if (bytes == sizeof(etc_data_t))
        {
            /* 由于 FATFS 的 rename 不支持覆盖已存在的文件，
               我们需要在重命名前，先手动删除（unlink）已经存在的旧文件 */
            unlink(file_path);
            if (rename(tmp_path, file_path) == 0)
            {
                result = RT_EOK;
            }
            else
            {
                unlink(tmp_path);
            }
        }
        else
        {
            unlink(tmp_path);
        }
    }
#else
    int free_index = -1;
    int exist_index = -1;

    for (int i = 0; i < RAM_DB_MAX_USERS; i++)
    {
        if (ram_user_db[i].is_valid && rt_strcmp(ram_user_db[i].uid, uid) == 0)
        {
            exist_index = i;
            break;
        }
        if (!ram_user_db[i].is_valid && free_index == -1)
        {
            free_index = i;
        }
    }

    int target_index = (exist_index != -1) ? exist_index : free_index;
    if (target_index != -1)
    {
        rt_strncpy(ram_user_db[target_index].uid, uid, sizeof(ram_user_db[target_index].uid) - 1);
        rt_memcpy(&ram_user_db[target_index].etc_data, data, sizeof(etc_data_t));
        ram_user_db[target_index].is_valid = RT_TRUE;
        result = RT_EOK;
    }
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

rt_err_t etc_storage_delete_user(const char *uid)
{
    rt_err_t result = -RT_ERROR;
    if (uid == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    char file_path[MAX_PATH_LEN];
    get_user_file_path(uid, file_path, sizeof(file_path));
    if (unlink(file_path) == 0)
    {
        result = RT_EOK;
    }
#else
    for (int i = 0; i < RAM_DB_MAX_USERS; i++)
    {
        if (ram_user_db[i].is_valid && rt_strcmp(ram_user_db[i].uid, uid) == 0)
        {
            ram_user_db[i].is_valid = RT_FALSE;
            rt_memset(&ram_user_db[i], 0, sizeof(ram_user_t));
            result = RT_EOK;
            break;
        }
    }
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

/* 核心特性：脱机交易日志的原子保存 */
rt_err_t etc_storage_save_offline_log(const char *uid, const etc_data_t *data)
{
    rt_err_t result = -RT_ERROR;
    if (uid == RT_NULL || data == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    char file_path[MAX_PATH_LEN];
    char tmp_path[MAX_PATH_LEN];

    rt_tick_t tick = rt_tick_get();
    rt_snprintf(file_path, sizeof(file_path), "%s/tx_%u.log", LOG_DIR_PATH, (unsigned int)tick);
    rt_snprintf(tmp_path, sizeof(tmp_path), "%s/tx_%u.tmp", LOG_DIR_PATH, (unsigned int)tick);

    etc_log_t log_entry;
    rt_memset(&log_entry, 0, sizeof(etc_log_t));
    rt_strncpy(log_entry.uid, uid, sizeof(log_entry.uid) - 1);
    rt_memcpy(&log_entry.data, data, sizeof(etc_data_t));

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0)
    {
        int bytes = write(fd, &log_entry, sizeof(etc_log_t));
        close(fd);
        if (bytes == sizeof(etc_log_t))
        {
            if (rename(tmp_path, file_path) == 0)
            {
                result = RT_EOK;
            }
            else
            {
                unlink(tmp_path);
            }
        }
        else
        {
            unlink(tmp_path);
        }
    }
#else
    rt_kprintf("[Storage] Warning: Offline log cache is bypassed in RAM fallback.\n");
    result = RT_EOK;
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

/* 提取最古老的一条未同步离线记录（队列形式） */
rt_err_t etc_storage_get_oldest_offline_log(etc_log_t *log_out, char *filename_out, rt_size_t name_len)
{
    rt_err_t result = -RT_ERROR;
    if (log_out == RT_NULL || filename_out == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    DIR *dir = opendir(LOG_DIR_PATH);
    if (dir != RT_NULL)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != RT_NULL)
        {
            if (rt_strstr(entry->d_name, ".log") != RT_NULL)
            {
                rt_snprintf(filename_out, name_len, "%s/%s", LOG_DIR_PATH, entry->d_name);

                int fd = open(filename_out, O_RDONLY);
                if (fd >= 0)
                {
                    int bytes = read(fd, log_out, sizeof(etc_log_t));
                    close(fd);
                    if (bytes == sizeof(etc_log_t))
                    {
                        result = RT_EOK;
                    }
                }
                break; /* 每次提取一条，保证上传的时序性 */
            }
        }
        closedir(dir);
    }
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

/* 同步成功后，删除对应的物理日志文件 */
rt_err_t etc_storage_delete_offline_log(const char *filename)
{
    rt_err_t result = -RT_ERROR;
    if (filename == RT_NULL) return -RT_EINVAL;

    rt_mutex_take(&storage_mutex, RT_WAITING_FOREVER);

#ifdef RT_USING_DFS
    if (unlink(filename) == 0)
    {
        result = RT_EOK;
    }
#endif

    rt_mutex_release(&storage_mutex);
    return result;
}

void etc_storage_register_defaults(void)
{
    etc_data_t user1 = {
        .balance = 200,
        .deduct_amount = 0.0,
        .detect_status = 0,
        .plate_num = "苏Cxxxxx",
        .vehicle_type = "Car"
    };

    etc_storage_write_user("107E945C", &user1);

    rt_kprintf("[Storage] Registered default test users successfully.\n");
}
