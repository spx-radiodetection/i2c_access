/** @file
 *  @brief fboverlayset main function
 *
 *  The file contains the parameter parsing and validation for the fboverlayset application.
 *
 *  @author James Covey-Crump
 *
 *  @copyright 2016 Radiodetection Limited.  All rights reserved.
 */

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"

#include <fcntl.h>              /* for open */
#include <unistd.h>             /* for close */
#include <linux/i2c.h>          /* for i2c IOCTL specific defintions */
#include <linux/i2c-dev.h>      /* for i2c message definitions */
#include <sys/ioctl.h>          /* for kernel IOCTL inteface */

/* keep these arrays off the stack */
static guchar i2c_write_buf[256];
static guchar i2c_read_buf[256];

static gboolean verbose = FALSE;

/**
 * Conversion function to turn a string into an int, with validation and 0x base parsing
 * @param  string String to convert
 * @param  value  The int value to returned
 * @return        TRUE if valid
 */
gboolean str2int (gchar *string, gint *value)
{
    guint64 value64;
    gchar *endptr = NULL;
    value64 = g_ascii_strtoull (string, &endptr, 0);
    *value = (gint)value64;
    return endptr != string;
}

/**
 * Main Function
 * @param  argc Number of arguments
 * @param  argv Array of argument strings
 * @return      EXIT_SUCCESS (0) if successful
 */
int main (int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;

    gint param_idx = 1;
    int app_status = EXIT_SUCCESS;

    gboolean version = FALSE;
    gboolean dry_run = FALSE;

    gchar *i2c_dev = NULL;
    gint i2c_device_addr = 0;
    gboolean i2c_force = FALSE;
    gint i2c_read_size = -1;
    gint i2c_write_payload_size = 0;

    GOptionEntry entries[] = {
        { "version", 0,   0, G_OPTION_ARG_NONE, &version, "output version", NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "print what is happening", NULL },
        { "dry-run", 0, 0, G_OPTION_ARG_NONE, &dry_run, "don't execute", NULL },

        { "read", 'r', 0, G_OPTION_ARG_INT, &i2c_read_size, "perform i2c read (in bytes)", "bytes2read" },
        { "force", 'f', 0, G_OPTION_ARG_NONE, &i2c_force, "access i2c devices controlled by a driver", NULL },
        { NULL }
    };

    context = g_option_context_new ("<i2c-dev-node> <addr> [byte0 [byte1 [...]]]");
    g_option_context_set_summary (context, "Performs simple reads and writes to 7bit i2c addresses.");
    g_option_context_set_description (context, "Examples:\n" \
                                      "  i2c_access /dev/i2c-0 0x2b 1 184 255     # i2c write of 1 184 255 to 0x2b on /dev/i2c-0\n" \
                                      "  i2c_access /dev/i2c-0 0x68 3 -r 1 -f     # i2c write of 3 to 0x68, follow by 1 byte read, forced\n" \
                                      "  i2c_access /dev/i2c-0 0x29 -r 2          # i2c read of 2 bytes from 0x29\n" \
                                      "\nFor issues contact " PACKAGE_BUGREPORT);
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

    if (version) {
        g_print ("%s\n", PACKAGE_STRING);
        exit (EXIT_SUCCESS);
    }

    /* Parameter Validation */
    if (argc < 3) {
        g_printerr ("%s\n", g_option_context_get_help (context, TRUE, NULL));
        exit (EXIT_FAILURE);
    }

    if (!g_str_has_prefix (argv[param_idx], "/dev/i2c")) {
        g_printerr ("Incorrect i2c device %s (expecting /dev/i2c...)\n", argv[param_idx]);
        exit (EXIT_FAILURE);
    }
    i2c_dev = argv[param_idx];
    param_idx++;

    if (!str2int (argv[param_idx], &i2c_device_addr) || (i2c_device_addr < 0x04) || (i2c_device_addr > 0x77)) {
        g_printerr ("Bad i2c device address %s  (0x04-0x77 valid)\n", argv[param_idx]);
        exit (EXIT_FAILURE);
    }
    param_idx++;

    while (param_idx < argc) {
        gint data;
        if (!str2int (argv[param_idx], &data) || (data > 255)) {
            g_printerr ("Data byte too big %s (bytes only)\n", argv[param_idx]);
            exit (EXIT_FAILURE);
        }
        i2c_write_buf[i2c_write_payload_size] = (guchar) data;
        i2c_write_payload_size++;
        param_idx++;
    }

    if (verbose) {
        int ii;
        if (i2c_write_payload_size > 0) {
            g_print ("Performing i2c write to address 0x%02x on bus %s\n", (unsigned)i2c_device_addr, i2c_dev);
        }
        for (ii = 0; ii < i2c_write_payload_size; ii++) {
            g_print (" 0x%02x", (unsigned)i2c_write_buf[ii]);
        }
        if (ii > 0) g_print ("\n");
        if (i2c_read_size >= 0) {
            g_print ("Performing i2c read of %d bytes from address 0x%02x on bus %s\n", (unsigned)i2c_read_size, (unsigned)i2c_device_addr, i2c_dev);
        }
    }

    if (i2c_write_payload_size == 0 && i2c_read_size < 0) {
        g_printerr ("no read requested, and no bytes to write - so no i2c transaction performed\n");
    }


    /* Execute */
    if (dry_run) {
        g_printerr ("Dry-run - no i2c transactions performed\n");
    } else {

        /*
         * i2c_dev          is the i2c device
         * i2c_device_addr  is the i2c remote device's address
         * i2c_payload_size holds the number of bytes to be written (0 means no i2c write)
         * i2c_write_buf    holds any bytes to be written
         * i2c_read_size    holds the number of bytes to read (0 sized reads ok, -1 means no i2c read)
         * i2c_read_buf     holds any bytes read
         */

        int i2c_file;
        struct i2c_rdwr_ioctl_data packets;
        struct i2c_msg messages[2];
        gint message_count = 0;

        i2c_file = open (i2c_dev, O_RDWR);

        if (i2c_file < 0) {
            g_printerr ("Unable to open i2c dev - %s", i2c_dev);
            app_status = EXIT_FAILURE;
        } else {
            /* add a transaction for the write (0 sized writes not performed) */
            if (i2c_write_payload_size > 0) {
                messages[message_count].addr  = (guint16) i2c_device_addr;
                messages[message_count].flags = 0;
                messages[message_count].len   = (guint16) i2c_write_payload_size;
                messages[message_count].buf   = i2c_write_buf;
                message_count++;
            }

            /* add a transaction for the read (0 sized reads accepted) */
            if (i2c_read_size >= 0) {
                messages[message_count].addr  = (guint16) i2c_device_addr;
                messages[message_count].flags = I2C_M_RD;   /* | I2C_M_NOSTART */
                messages[message_count].len   = (guint16) i2c_read_size;
                messages[message_count].buf   = i2c_read_buf;
                message_count++;
            }

            packets.msgs      = messages;
            packets.nmsgs     = message_count;

            /* With force, let the user read from/write to the registers even when a driver is also running
             * NOTE the slave address is overridden by the addresses in the messages above. */
            if (ioctl (i2c_file, i2c_force ? I2C_SLAVE_FORCE : I2C_SLAVE, i2c_device_addr) < 0) {
                g_printerr ("Could not set address to 0x%02x (address in use?)\n", i2c_device_addr);
                app_status = EXIT_FAILURE;
            } else if (ioctl (i2c_file, I2C_RDWR, &packets) < 0) {
                g_printerr ("i2c driver error - unable to perform transaction\n");
                app_status = EXIT_FAILURE;
            } else {
                int ii;
                for (ii = 0; ii < i2c_write_payload_size; ii++) {
                    g_print (" 0x%02x", (unsigned)i2c_read_buf[ii]);
                }
                if (ii > 0) g_printerr ("\n");
            }

            close (i2c_file);
        }

    }

    return (app_status);
}
