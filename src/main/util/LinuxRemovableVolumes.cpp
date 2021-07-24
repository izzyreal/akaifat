#if defined (__linux__)
#include "RemovableVolumes.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <fcntl.h>

using namespace akaifat::util;

static const char *BLOCK_PATH = "/org/freedesktop/UDisks2/block_devices/";

std::vector<char> getDriveLetters()
{
    return {};
}

bool IsRemovable(char driveLetter)
{
    return false;
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");

    if (!pipe) return "";

    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
            result += buffer;
    } catch (const std::exception&) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

uint64_t get_media_size(std::string bsdName)
{
    int64_t mediaSize = 0;

    std::string cmd = "lsblk -b --output SIZE -n -d " + bsdName;

    auto mediaSizeStr = exec(cmd.c_str());

    try {
        mediaSize = std::stoi(mediaSizeStr);
    } catch (const std::exception&) {
        // nothing to do
    }

    printf("Reported media size: %i\n", mediaSize);

    return mediaSize;
}

void RemovableVolumes::on_object_added(GDBusObjectManager *manager,
                            GDBusObject *dbus_object, gpointer user_data) {
    UDisksObject *object = NULL;
    UDisksBlock *block = NULL;
    UDisksFilesystem *filesystem = NULL;
    RemovableVolumes* that = (RemovableVolumes*) user_data;

    const char *path = g_dbus_object_get_object_path(dbus_object);
    //fprintf(stderr, "New object: %s: ", path);

    if (strncmp(path, BLOCK_PATH, strlen(BLOCK_PATH)) != 0) {
        //fprintf(stderr, "Not a block device\n");
        return;
    }

    object = UDISKS_OBJECT(dbus_object);

    block = udisks_object_peek_block(object);

    if (block == NULL) return;

    filesystem = udisks_object_peek_filesystem(object);

    if (filesystem == NULL) return;

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&builder, "{sv}", "auth.no_user_interaction", g_variant_new_boolean(TRUE));

    GVariant *options = g_variant_builder_end (&builder);
    g_variant_ref_sink (options);

    gchar *mount_path = NULL;
    GError *error = NULL;

    auto bsdName = "/dev/" + std::string(path).substr(strlen(BLOCK_PATH));

    bool is_raw_accessible = false;

    if (!udisks_filesystem_call_mount_sync(
                filesystem, options, &mount_path, NULL, &error)) {
        g_error_free(error);
    } else {
        printf("Able to mount\n");
        is_raw_accessible = true;

        if (!udisks_filesystem_call_unmount_sync(
                filesystem, options, NULL, &error)) {
            fprintf(stderr, "Error unmounting: %s\n", error->message);
            g_error_free(error);
        } else {
            is_raw_accessible = true;
        }

        g_free(mount_path);
    }
    g_variant_unref(options);

    if (!is_raw_accessible) return;

            for (auto& l : that->listeners)
                l->processChange(bsdName, get_media_size(bsdName));
}

void RemovableVolumes::init()
{
    running = true;

    changeListenerThread = std::thread([&]{

        GError *error = NULL;
        UDisksClient *client = udisks_client_new_sync(NULL, &error);

        if (client == NULL) {
            fprintf(stderr, "Error connecting to the udisks daemon: %s\n", error->message);
            g_error_free(error);
            return;
        }

        GDBusObjectManager *manager = udisks_client_get_object_manager(client);

        GList* objects = g_dbus_object_manager_get_objects(manager);
        GList *l;

        for (l = objects; l != NULL; l = g_list_next (l))
        {
            GDBusObject* obj = G_DBUS_OBJECT(l->data);

            if (!obj) continue;

            on_object_added(manager, obj, this);
        }

        g_list_free_full (objects, g_object_unref);

        g_signal_connect(manager, "object-added", G_CALLBACK(on_object_added), this);

        while (running)
        {
            g_main_context_iteration(g_main_context_default(), false);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        g_object_unref(client);
    });
}
#endif
