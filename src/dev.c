//#include <linux/idr.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

struct User {
    // owned, non-null
    char *name;
    // owned, optional
    char *age;
    // owned, optional
    char *phone_number;
    // owned, optional
    char *email;
    struct list_head links;
};

static void user_free(struct User u) {

    kfree(u.name);
    kfree(u.email);

    kfree(u.age);
    kfree(u.phone_number);
}

struct Phonebook {
    // head of the users list
    struct list_head users;
} PHONEBOOK;

static void phonebook_init(struct Phonebook *p) { INIT_LIST_HEAD(&p->users); }

static void phonebook_add_user(struct Phonebook *p, struct User *user) {
    printk(KERN_INFO "phonebook: adding new user %s", user->name);
    if (user->age != NULL) {
        printk(KERN_INFO "phonebook: age = %d", *user->age);
    }
    if (user->phone_number != NULL) {
        printk(KERN_INFO "phonebook: phone_number = %s", user->phone_number);
    }
    if (user->email != NULL) {
        printk(KERN_INFO "phonebook: email = %s", user->email);
    }
    INIT_LIST_HEAD(&user->links);
    list_add(&user->links, &p->users);
}

struct User *phonebook_find_user(struct Phonebook *p, char const *name) {
    struct User *item;
    printk(KERN_INFO "phonebook: searching for user %s", name);

    list_for_each_entry(item, &p->users, links) {
        printk(KERN_INFO "user: %s", item->name);
        if (strcmp(item->name, name) == 0) {
            printk(KERN_INFO "found!");
            return item;
        }
    }
    printk(KERN_INFO "not found!");

    return NULL;
}

static void phonebook_delete_user(struct Phonebook *p, char const *name) {
    struct User *item;
    printk(KERN_INFO "phonebook: deleting user %s", name);

    list_for_each_entry(item, &p->users, links) {
        printk(KERN_INFO "user: %s", item->name);
        if (strcmp(item->name, name) == 0) {
            printk(KERN_INFO "user deleted");
            list_del_init(&item->links);
            user_free(*item);
            return;
        }
    }
    printk(KERN_INFO "user did not exist");
}

#define MAX_COMMAND_LEN 1024

/// Session is command interpeter, tied to concrete file
struct Session {
    struct Phonebook *pb;
    // prefix of current command
    char cmd[MAX_COMMAND_LEN + 1];
    // our last reply
    char *reply;
    size_t reply_pos;
};

static ssize_t phonebook_device_read(struct file *file, char *buf, size_t len,
                                     loff_t *offset) {
    struct Session *sess;
    size_t reply_size;
    size_t cnt;
    sess = (struct Session *)file->private_data;
    if (!sess->reply) {
        printk(KERN_INFO "phonebook: no reply queued");
        return 0;
    }
    reply_size = strlen(sess->reply);
    cnt = min(len, reply_size - sess->reply_pos);
    printk(KERN_INFO "phonebook: queued reply info: total_len=%zu, already_returned=%zu, read_sz=%zu", reply_size, sess->reply_pos, cnt);
    if (cnt == 0) {
        return 0;
    }
    if (copy_to_user(buf, sess->reply + sess->reply_pos, cnt)) {
        return -EFAULT;
    }
    sess->reply_pos += cnt;
    return cnt;
}

static int execute_creation_command(struct Session *sess, char const *s) {
    struct User *data;
    int ret = 0;
    int const PARSE_STATE_KEY = 1;
    int const PARSE_STATE_VALUE = 2;
    int parse_state = PARSE_STATE_KEY;
    char key = '\0';
    char const *value_begin = NULL;
    size_t value_len;
    char *value_copy;
    char **dest_ptr;

    data = kmalloc(sizeof(struct User), GFP_KERNEL);
    data->name = NULL;
    data->age = NULL;
    data->phone_number = NULL;
    data->email = NULL;

    while (true) {
        if (parse_state == PARSE_STATE_KEY) {
            if (!*s) {
                printk(KERN_INFO "expected key, got eof");
                ret = -EINVAL;
                goto cleanup;
            }
            key = *s;
            parse_state = PARSE_STATE_VALUE;
            value_begin = s + 1;
        } else {
            if (*s == '\0' || *s == ':') {
                parse_state = PARSE_STATE_KEY;
                value_len = s - value_begin;
                value_copy = kmalloc(value_len + 1, GFP_KERNEL);
                strncpy(value_copy, value_begin, value_len);
                value_copy[value_len] = '\0';
                dest_ptr = NULL;
                switch (key) {
                case 'n':
                    printk(KERN_INFO "filling name field");
                    dest_ptr = &data->name;
                    break;
                case 'a':
                    printk(KERN_INFO "filling age field");
                    dest_ptr = &data->age;
                    break;
                case 'e':
                    printk(KERN_INFO "filling email field");
                    dest_ptr = &data->email;
                    break;
                case 'p':
                    printk(KERN_INFO "filling phone_number field");
                    dest_ptr = &data->phone_number;
                    break;
                default:
                    break;
                }
                if (dest_ptr == NULL) {
                    printk(KERN_INFO "unknown field: %c", key);
                    ret = -EINVAL;
                    goto cleanup;
                }
                printk(KERN_INFO "parsed value: %s", value_copy);
                *dest_ptr = value_copy;
            }
            if (!*s) {
    printk(KERN_INFO "all fields are parsed");
                break;
            }
        }
        ++s;
    }

    if (data->name == NULL) {
        printk(KERN_INFO "user name not provided");
        ret = -EINVAL;
        goto cleanup;
    }

    if (phonebook_find_user(sess->pb, data->name)) {
        printk(KERN_INFO "user already exists");
        ret = -EEXIST;
        goto cleanup;
    }

    phonebook_add_user(sess->pb, data);
    return 0;

cleanup:
    user_free(*data);
    return ret;
}

static void try_add_field(char **reply_buf, size_t *reply_len, char const *k,
                          char const *v) {
    size_t cnt;

if (!v) {
    return;
}
    cnt = snprintf(*reply_buf, *reply_len, "%s=%s\n", k, v);
    *reply_buf += cnt;
    *reply_len -= cnt;
}

static void execute_lookup_command(struct Session *sess, char const *name,
                                   char *reply_buf, size_t reply_len) {
    struct User *data;

    data = phonebook_find_user(sess->pb, name);
    if (!data) {
        snprintf(reply_buf, reply_len, "user not found: %s\n", name);
        return;
    }
    try_add_field(&reply_buf, &reply_len, "name", data->name);
    try_add_field(&reply_buf, &reply_len, "age", data->age);
    try_add_field(&reply_buf, &reply_len, "email", data->email);
    try_add_field(&reply_buf, &reply_len, "phone_number", data->phone_number);
}

static void execute_delete_command(struct Session *sess, char const *name) {
    phonebook_delete_user(sess->pb, name);
}

#define MAX_REPLY_SIZE 1024

static int execute_command(struct Session *sess, char const *command) {
    size_t command_len;
    printk("Executing command %s", command);
    // cleanup old reply
    if (sess->reply) {
        kfree(sess->reply);
        sess->reply = NULL;
    }
    command_len = strlen(command);
    if (command_len == 0) {
        printk(KERN_INFO "command is empty");
        return -EINVAL;
    }
    printk(KERN_INFO "requested action: %c", command[0]);
    switch (command[0]) {
    case 'C':
        return execute_creation_command(sess, command + 1);
    case 'V':
        sess->reply = kmalloc(MAX_REPLY_SIZE + 1, GFP_KERNEL);
        sess->reply_pos = 0;
        memset(sess->reply, 0, MAX_REPLY_SIZE + 1);
        execute_lookup_command(sess, command + 1, sess->reply, MAX_REPLY_SIZE);
        printk(KERN_INFO "created a reply of size %zu", strlen(sess->reply));
        break;
    case 'D':
        execute_delete_command(sess, command + 1);
        break;
    default:
        printk(KERN_INFO "unknown action");
        return -EINVAL;
    }

    return 0;
}

static ssize_t phonebook_device_write(struct file *file, char const *buf,
                                      size_t len, loff_t *offset) {
    struct Session *sess;
    char *command_fin;
    int err;
    size_t current_len;
    sess = (struct Session *)(file->private_data);
    current_len = strlen(sess->cmd);
    if (current_len + len > MAX_COMMAND_LEN) {
        return -EINVAL;
    }
    if (copy_from_user(sess->cmd + current_len, buf, len)) {
        return -EFAULT;
    }
    sess->cmd[current_len + len] = '\0';
    printk(KERN_INFO "phonebook: command buffer len is now %zu",
           current_len + len);
    command_fin = strchr(sess->cmd, '\n');
    if (command_fin == NULL) {
        return len;
    }
    *command_fin = '\0';
    err = execute_command(sess, sess->cmd);
    sess->cmd[0] = '\0';
    if (err) {
        return err;
    }

    return len;
}

static int open_files_count = 0;

static int phonebook_device_open(struct inode *inode, struct file *file) {
    struct Session *sess;

    if (open_files_count != 0) {
        printk(KERN_INFO "phonebook does not support parallel access yet :(");
        return -EBUSY;
    }
    ++open_files_count;
    try_module_get(THIS_MODULE);
    sess = kmalloc(sizeof(struct Session), GFP_KERNEL);
    sess->pb = &PHONEBOOK;
    sess->cmd[0] = '\0';
    sess->reply = NULL;
    file->private_data = sess;
    return 0;
}
static int phonebook_device_release(struct inode *inode, struct file *file) {
    --open_files_count;
    module_put(THIS_MODULE);
    return 0;
}

static int major_num;

static int phonebook_device_register(void) {
    static struct file_operations fops = {.open = phonebook_device_open,
                                          .release = phonebook_device_release,
                                          .read = phonebook_device_read,
                                          .write = phonebook_device_write};
    major_num = register_chrdev(0, "phonebook", &fops);
    if (major_num < 0) {
        return major_num;
    }
    printk(KERN_INFO "registered device: major=%d", major_num);
    return 0;
}

static void phonebook_device_unregister(void) {
    unregister_chrdev(major_num, "phonebook");
}

static int __init phonebook_mod_init(void) {
    int err;
    printk(KERN_INFO "hello world!\n");
    err = phonebook_device_register();
    if (err) {
        printk(KERN_ALERT "unable to register phonebook device: %d", err);
    }
    printk(KERN_INFO "creating global phonebook\n");
    phonebook_init(&PHONEBOOK);
    return 0;
}

static void __exit phonebook_mod_exit(void) {
    phonebook_device_unregister();
    printk(KERN_INFO "goodbye world\n");
}

module_init(phonebook_mod_init);
module_exit(phonebook_mod_exit);