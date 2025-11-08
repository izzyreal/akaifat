#if defined (__linux__)
#include "RemovableVolumes.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <fcntl.h>

using namespace akaifat::util;

static const char *BLOCK_PATH = "/org/freedesktop/UDisks2/block_devices/";

std::vector<char> getDriveLetters() { return {}; }

bool IsRemovable(char driveLetter) { return false; }

std::string exec(const char* cmd)
{
    char buffer[128];
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";

    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
            result += buffer;
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

std::string get_filesystem_type(const std::string& bsdName)
{
    auto result = exec(("lsblk -b -o fsver -n -d " + bsdName).c_str());
    if (!result.empty() && result.back() == '\n') result.pop_back();
    printf("Reported filesystem type: %s\n", result.c_str());
    return result;
}

std::string get_volume_uuid(const std::string& bsdName)
{
    auto result = exec(("lsblk -b -o uuid -n -d " + bsdName).c_str());
    if (!result.empty() && result.back() == '\n') result.pop_back();
    printf("Reported UUID: %s\n", result.c_str());
    return result;
}

std::string get_volume_label(const std::string& bsdName)
{
    auto result = exec(("lsblk -b -o label -n -d " + bsdName).c_str());
    if (!result.empty() && result.back() == '\n') result.pop_back();
    printf("Reported label: %s\n", result.c_str());
    return result;
}

uint64_t get_media_size(const std::string& bsdName)
{
    uint64_t mediaSize = 0;
    auto mediaSizeStr = exec(("lsblk -b -o SIZE -n -d " + bsdName).c_str());
    try {
        mediaSize = std::stoull(mediaSizeStr);
    } catch (...) {}
    printf("Reported media size: %lu\n", mediaSize);
    return mediaSize;
}

void RemovableVolumes::on_object_added(GDBusObjectManager *manager,
                                       GDBusObject *dbus_object,
                                       gpointer user_data)
{
    auto that = static_cast<RemovableVolumes*>(user_data);
    const char *path = g_dbus_object_get_object_path(dbus_object);
    if (strncmp(path, BLOCK_PATH, strlen(BLOCK_PATH)) != 0) return;

    UDisksObject *object = UDISKS_OBJECT(dbus_object);
    UDisksBlock *block = udisks_object_peek_block(object);
    if (!block) return;

    UDisksFilesystem *filesystem = udisks_object_peek_filesystem(object);
    if (!filesystem) return;

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&builder, "{sv}", "auth.no_user_interaction", g_variant_new_boolean(TRUE));

    GVariant *options = g_variant_builder_end(&builder);
    g_variant_ref_sink(options);

    gchar *mount_path = NULL;
    GError *error = NULL;
    auto bsdName = "/dev/" + std::string(path).substr(strlen(BLOCK_PATH));
    bool is_raw_accessible = false;

    if (!udisks_filesystem_call_mount_sync(filesystem, options, &mount_path, NULL, &error)) {
        g_error_free(error);
    } else {
        if (!udisks_filesystem_call_unmount_sync(filesystem, options, NULL, &error)) {
            fprintf(stderr, "Error unmounting: %s\n", error->message);
            g_error_free(error);
        } else {
            is_raw_accessible = true;
        }
        g_free(mount_path);
    }
    g_variant_unref(options);
    if (!is_raw_accessible) return;

    auto filesystemType = get_filesystem_type(bsdName);
    if (filesystemType != "FAT16") return;

    auto volumeName = get_volume_label(bsdName);
    auto volumeUUID = get_volume_uuid(bsdName);
    auto mediaSize = get_media_size(bsdName);

    std::vector<VolumeChangeListener*> snapshot;
    {
        std::lock_guard<std::mutex> lk(that->listenersMutex);
        snapshot = that->listeners;
    }

    for (auto* l : snapshot)
        l->processChange(RemovableVolume{volumeUUID, bsdName, volumeName, mediaSize});
}

void RemovableVolumes::init()
{
    std::lock_guard<std::mutex> lk(listenersMutex);
    if (running.load()) return;
    running.store(true);

    changeListenerThread = std::thread([this] {
        GError *error = NULL;
        UDisksClient *client = udisks_client_new_sync(NULL, &error);
        if (!client) {
            fprintf(stderr, "Error connecting to the udisks daemon: %s\n", error->message);
            g_error_free(error);
            return;
        }

        GDBusObjectManager *manager = udisks_client_get_object_manager(client);

        GList* objects = g_dbus_object_manager_get_objects(manager);
        for (GList *l = objects; l; l = g_list_next(l)) {
            if (G_DBUS_OBJECT(l->data))
                on_object_added(manager, G_DBUS_OBJECT(l->data), this);
        }
        g_list_free_full(objects, g_object_unref);

        g_signal_connect(manager, "object-added", G_CALLBACK(on_object_added), this);

        while (running.load()) {
            g_main_context_iteration(g_main_context_default(), false);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        g_object_unref(client);
    });
}
#endif
