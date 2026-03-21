/*******************************************************
 HIDAPI - Multi-Platform library for
 having HID devices on multiple platforms.
*******************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

struct hid_device_;
typedef struct hid_device_ hid_device;

struct hid_device_info {
    char *path;
    unsigned short vendor_id;
    unsigned short product_id;
    wchar_t *serial_number;
    unsigned short release_number;
    wchar_t *manufacturer_string;
    wchar_t *product_string;
    unsigned short usage_page;
    unsigned short usage;
    int interface_number;
    struct hid_device_info *next;
};

int hid_init(void);
int hid_exit(void);

struct hid_device_info* hid_enumerate(unsigned short vendor_id, unsigned short product_id);
void hid_free_enumeration(struct hid_device_info *devs);

hid_device* hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number);
hid_device* hid_open_path(const char *path);

int hid_write(hid_device *dev, const unsigned char *data, size_t length);
int hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds);
int hid_read(hid_device *dev, unsigned char *data, size_t length);
int hid_set_nonblocking(hid_device *dev, int nonblock);

int hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length);
int hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length);

void hid_close(hid_device *dev);

int hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen);
int hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen);
int hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen);
int hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen);

const wchar_t* hid_error(hid_device *dev);

#ifdef __cplusplus
}
#endif
