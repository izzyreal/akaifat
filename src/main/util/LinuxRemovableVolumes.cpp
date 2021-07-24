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
    if (block == NULL) {
        //fprintf(stderr, "Not a block object\n");
        return;
    }

    filesystem = udisks_object_peek_filesystem(object);
    if (filesystem == NULL) {
        //fprintf(stderr, "Not a mountable filesystem\n");
        return;
    }

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&builder, "{sv}", "auth.no_user_interaction",
                          g_variant_new_boolean(TRUE));

    GVariant *options = g_variant_builder_end (&builder);
    g_variant_ref_sink (options);

    gchar *mount_path = NULL;
    GError *error = NULL;

    if (!udisks_filesystem_call_mount_sync(
                filesystem, options, &mount_path, NULL, &error)) {
        //fprintf(stderr, "Error mounting: %s\n", error->message);
        g_error_free(error);
    } else {
        //fprintf(stderr, "Mounting device at: %s\n", mount_path);

        if (!udisks_filesystem_call_unmount_sync(
                filesystem, options, NULL, &error)) {
            //fprintf(stderr, "Error unmounting: %s\n", error->message);
            g_error_free(error);
        } else {
            //fprintf(stderr, "Unmounted device at: %s\n", mount_path);
            auto bsdName = "/dev/" + std::string(path).substr(strlen(BLOCK_PATH));

            int file_descriptor = open(bsdName.c_str(), O_RDONLY);

            int64_t mediaSize;

            ioctl(file_descriptor, BLKGETSIZE64, &mediaSize);

            printf("Reported media size: %ul\n", mediaSize);

            for (auto& l : that->listeners)
                l->processChange(bsdName, mediaSize);

            close(file_descriptor);
        }

        g_free(mount_path);
    }
    g_variant_unref(options);
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
