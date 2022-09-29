/*
 ============================================================================
 Name        : hidirt.c
 Author      : pikim
 Version     :
 Copyright   : GPL v3
 Description : HIDIRT console interface application
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <libconfig.h>
#include <xdo.h>

#include "hidapi.h"


#define HIDIRT_VID 0x0483 // for testing only
#define HIDIRT_PID 0x6611 // for testing only

#define MAX_FEATURE_REPORT_LENGTH 16
#define MAX_STRING_LENGTH 255

static const char* config_file = "hidirt.cfg";
static hid_device* handle;
static config_t cfg;
static xdo_t* x;


enum ReportID {
	IrCodeInterrupt     = 1,
	ReadFirmwareVersion = 0x10,
	ControlPcEnable     = 0x11,
	ForwardIrEnable     = 0x12,
	PowerOnCode         = 0x13,
	PowerOffCode        = 0x14,
	ResetCode           = 0x15,
	MinRepeats          = 0x16,
	DeviceTime          = 0x17,
	ClockDeviation      = 0x18,
	WakeupTime          = 0x19,
	WakeupTimeSpan      = 0x1a,
	RequestBootloader   = 0x50,
	WatchdogEnable      = 0x51,
	WatchdogReset       = 0x52
};

struct __attribute__((__packed__)) ircode {
	unsigned char  protocol; // protocol, e.g. NEC_PROTOCOL
	unsigned short address;  // address
	unsigned short command;  // command
	unsigned char  flags;    // flags, e.g. repetition
};


int feature_bool(hid_device *handle, unsigned char report_id, char *arg) {
	int res;
	unsigned char buf[2];

	buf[0] = report_id;
	if (arg && *arg) {
		// argument present -> write report
		// convert string to int and send report
		buf[1] = atoi(&arg[1]); // arg[0] is '='
		res = hid_send_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// writing report failed
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -1;
		}
		return 0;
	}
	else {
		// no argument present -> read report
		res = hid_get_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// reading report failed
			fprintf(stderr, "Error reading ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		// print result
		fprintf(stdout, "%d\n", buf[1]);
		return 0;
	}
}


int feature_ircode(hid_device *handle, unsigned char report_id, char *arg) {
	int res, idx, value;
	unsigned char buf[7];
	struct ircode code;
	char delim[] = ",;-";
	char *substr, *x_pos;

	memset(buf, 0, sizeof(buf));
	buf[0] = report_id;
	if (arg && *arg) {
		// argument present -> write report
		idx = 1;
		// split string, return first token
		substr = strtok(arg, delim);
		while (substr != NULL) {
			// as long the end wasn't reached
			switch (idx) {
				// handle data depending on the destination index
				case 1:
				case 6:
					// determine if value was given in hex notation
					x_pos = strchr(substr, 'x');
					if (x_pos == NULL) {
						// decimal notation
						if (idx == 1) {
							// for the first token
							substr += 1; // substr[0] is '='
						}
						buf[idx] = atoi(substr);
					}
					else {
						// hex notation
						buf[idx] = strtol(x_pos+1, NULL, 16);
					}
					idx += 1;
					break;
				case 2:
				case 4:
					// determine if value was given in hex notation
					x_pos = strchr(substr, 'x');
					if (x_pos == NULL) {
						// decimal notation
						value = atoi(substr);
					}
					else {
						// hex notation
						value = strtol(x_pos+1, NULL, 16);
					}
					buf[idx] = value;
					buf[idx+1] = value >> 8;
					idx += 2;
					break;
			}
			// get next token
			substr = strtok(NULL, delim);
		}
		res = hid_send_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -1;
		}
		return 0;
	}
	else {
		// no argument present -> read report
		res = hid_get_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			fprintf(stderr, "Error reading ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		// prepare and print result
		code.protocol = buf[1];
		code.address = buf[3];
		code.address <<= 8;
		code.address += buf[2];
		code.command = buf[5];
		code.command <<= 8;
		code.command += buf[4];
		code.flags = buf[6];
		fprintf(stdout, "0x%02hhx,0x%04hx,0x%04hx,0x%02hhx\n",
				code.protocol, code.address, code.command, code.flags);
		return 0;
	}
}


int feature_devicetime(hid_device *handle, unsigned char report_id, char *arg) {
	int res, secs, msecs;
	unsigned char buf[7];

	buf[0] = report_id;
	if (arg && *arg) {
		// argument present -> write report
		// convert string to int and send report
//		buf[1] = atoi(&arg[1]); // arg[0] is '='
//		res = hid_send_feature_report(handle, buf, sizeof(buf));
		res = 1;
		fprintf(stdout, "Writing ReportID %d not implemented.\n", buf[0]);
		if (res < 1) {
			// writing report failed
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -1;
		}
		return 0;
	}
	else {
		// no argument present -> read report
		res = hid_get_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// reading report failed
			fprintf(stderr, "Error reading ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		// prepare and print result
		secs = buf[4];
		secs <<= 8;
		secs += buf[3];
		secs <<= 8;
		secs += buf[2];
		secs <<= 8;
		secs += buf[1];
		msecs = buf[6];
		msecs <<= 8;
		msecs += buf[5];
		msecs /= 10;
		fprintf(stdout, "%d.%d\n", secs, msecs);
		return 0;
	}
}


int feature_timedeviation(hid_device *handle, unsigned char report_id, char *arg) {
	int res, value;
	unsigned char buf[5];

	buf[0] = report_id;
	if (arg && *arg) {
		// argument present -> write report
		// convert string to int and send report
		value = atoi(&arg[1]); // arg[0] is '='
		buf[1] = value;
		value >>= 8;
		buf[2] = value;
		value >>= 8;
		buf[3] = value;
		value >>= 8;
		buf[4] = value;
//		res = hid_send_feature_report(handle, buf, sizeof(buf));
		res = 1;
		if (res < 1) {
			// writing report failed
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -1;
		}
		return 0;
	}
	else {
		// no argument present -> read report
		res = hid_get_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// reading report failed
			fprintf(stderr, "Error reading ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		// prepare and print result
		value = buf[4];
		value <<= 8;
		value += buf[3];
		value <<= 8;
		value += buf[2];
		value <<= 8;
		value += buf[1];
		fprintf(stdout, "%d\n", value);
		return 0;
	}
}


int feature_waketime(hid_device *handle, unsigned char report_id, char *arg) {
	int res, value;
	unsigned char buf[5];

	buf[0] = report_id;
	if (arg && *arg) {
		// argument present -> write report
		// convert string to int and send report
		value = atoi(&arg[1]); // arg[0] is '='
		buf[1] = value;
		value >>= 8;
		buf[2] = value;
		value >>= 8;
		buf[3] = value;
		value >>= 8;
		buf[4] = value;
		res = hid_send_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// writing report failed
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -1;
		}
		return 0;
	}
	else {
		// no argument present -> read report
		res = hid_get_feature_report(handle, buf, sizeof(buf));
		if (res < 1) {
			// reading report failed
			fprintf(stderr, "Error reading ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		// prepare and print result
		value = buf[4];
		value <<= 8;
		value += buf[3];
		value <<= 8;
		value += buf[2];
		value <<= 8;
		value += buf[1];
		fprintf(stdout, "%d\n", value);
		return 0;
	}
}


int send_ircode(hid_device *handle, char *arg) {
	int res, idx, value;
	unsigned char buf[7];
	char delim[] = ",;-";
	char *substr, *x_pos;

	memset(buf, 0, sizeof(buf));
	buf[0] = IrCodeInterrupt;
	if (arg && *arg) {
		// argument present -> write report
		idx = 1;
		// split string, return first token
		substr = strtok(arg, delim);
		while (substr != NULL) {
			// as long the end wasn't reached
			switch (idx) {
				// handle data depending on the destination index
				case 1:
				case 6:
					// determine if value was given in hex notation
					x_pos = strchr(substr, 'x');
					if (x_pos == NULL) {
						// decimal notation
						if (idx == 1) {
							// for the first token
							substr += 1; // substr[0] is '='
						}
						buf[idx] = atoi(substr);
					}
					else {
						// hex notation
						buf[idx] = strtol(x_pos+1, NULL, 16);
					}
					idx += 1;
					break;
				case 2:
				case 4:
					// determine if value was given in hex notation
					x_pos = strchr(substr, 'x');
					if (x_pos == NULL) {
						// decimal notation
						value = atoi(substr);
					}
					else {
						// hex notation
						value = strtol(x_pos+1, NULL, 16);
					}
					buf[idx] = value;
					buf[idx+1] = value >> 8;
					idx += 2;
					break;
			}
			// get next token
			substr = strtok(NULL, delim);
		}
		res = hid_write(handle, buf, sizeof(buf));
		if (res < 1) {
			fprintf(stderr, "Error writing ReportID: %d. Errorcode: %d\n", buf[0], res);
			return -2;
		}
		return 0;
	}
	return -1;
}


int show_device_details(hid_device *handle) {
	int res;
	unsigned char buf[MAX_FEATURE_REPORT_LENGTH];
	wchar_t wstr[MAX_STRING_LENGTH];

	// read the firmware revision
	buf[0] = ReadFirmwareVersion;
	res = hid_get_feature_report(handle, buf, sizeof(buf));
	if (res < 1) {
		fprintf(stderr, "Error reading firmware revision. Errorcode: %d\n", res);
		return res;
	}
	fprintf(stdout, "Firmware Revision: %s\n", buf);

	// read the Manufacturer String
	res = hid_get_manufacturer_string(handle, wstr, MAX_STRING_LENGTH);
	if (res == 0) { // 0 means no error occurred
		fprintf(stdout, "Manufacturer String: %ls\n", wstr);
	}
	else {
		fprintf(stderr, "hid_get_manufacturer_string() failed. Errorcode: %d\n", res);
		return res;
	}

	// read the Product String
	res = hid_get_product_string(handle, wstr, MAX_STRING_LENGTH);
	if (res == 0) { // 0 means no error occurred
		fprintf(stdout, "Product String: %ls\n", wstr);
	}
	else {
		fprintf(stderr, "hid_get_product_string() failed. Errorcode: %d\n", res);
		return res;
	}

	// read the Serial Number String
	res = hid_get_serial_number_string(handle, wstr, MAX_STRING_LENGTH);
	if (res == 0) { // 0 means no error occurred
		fprintf(stdout, "Serial Number String: (%d) %ls\n", wstr[0], wstr);
	}
	else {
		fprintf(stderr, "hid_get_serial_number_string() failed. Errorcode: %d\n", res);
		return res;
	}

	return 0;
}


int create_config_file(void) {
	config_t cfg;
	config_setting_t *root, *name, *setting, *settings, *mapping, *mappings;

	config_init(&cfg);
	root = config_root_setting(&cfg);

	// add name
	name = config_setting_add(root, "name", CONFIG_TYPE_STRING);
	config_setting_set_string(name, "hidirt.cfg");

	// add some sections to the configuration
	settings = config_setting_add(root, "settings", CONFIG_TYPE_GROUP);
	mappings = config_setting_add(root, "mappings", CONFIG_TYPE_LIST);

	// add some settings
	setting = config_setting_add(settings, "send_keys", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, false);

	setting = config_setting_add(settings, "start_apps", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, false);

	setting = config_setting_add(settings, "sync_clocks", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, false);

	setting = config_setting_add(settings, "pc_clock_is_origin", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, true);

	setting = config_setting_add(settings, "calibration_start_time", CONFIG_TYPE_INT64);
	config_setting_set_int64(setting, 0);

	// add a mapping to the list
	mapping = config_setting_add(mappings, NULL, CONFIG_TYPE_GROUP);

	setting = config_setting_add(mapping, "description", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "any helpful name");

	setting = config_setting_add(mapping, "ir_protocol", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x01);

	setting = config_setting_add(mapping, "ir_address", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x02);

	setting = config_setting_add(mapping, "ir_command", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x03);

	setting = config_setting_add(mapping, "key", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# any key or key sequence to be sent");

	setting = config_setting_add(mapping, "application", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# path to an application to be started");

	setting = config_setting_add(mapping, "parameter", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# parameter for the application");

	// add another mapping to the list
	mapping = config_setting_add(mappings, NULL, CONFIG_TYPE_GROUP);

	setting = config_setting_add(mapping, "description", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "another helpful name");

	setting = config_setting_add(mapping, "ir_protocol", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x11);

	setting = config_setting_add(mapping, "ir_address", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x12);

	setting = config_setting_add(mapping, "ir_command", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 0x13);

	setting = config_setting_add(mapping, "key", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# another key or key sequence to be sent");

	setting = config_setting_add(mapping, "application", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# path to another application to be started");

	setting = config_setting_add(mapping, "parameter", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "# parameter for the other application");

	// write the new configuration to file
	if (!config_write_file(&cfg, config_file)) {
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return(-1);
	}

	fprintf(stderr, "New configuration successfully written to: %s\n", config_file);
	fprintf(stderr, "Restart application to reload config file.\n");

	config_destroy(&cfg);
	return(0);
}


void handle_ir_code(struct ircode* ir_code) {
	config_setting_t *mappings;

	// read settings
	int send_keys = false;
	config_lookup_bool(&cfg, "settings.send_keys", &send_keys);

	int start_apps = false;
	config_lookup_bool(&cfg, "settings.start_apps", &start_apps);

	// get all the mappings
	mappings = config_lookup(&cfg, "mappings");
	if (mappings != NULL) {
		unsigned int count = config_setting_length(mappings);
		unsigned int i;

		for (i = 0; i < count; i++) {
			// get one single mapping
			config_setting_t *mapping = config_setting_get_elem(mappings, i);
			int protocol, address, command;
			const char *key, *application, *parameter;

			// if any of the settings doesn't exist, proceed to next mapping
			if ( !(config_setting_lookup_int(mapping, "ir_protocol", &protocol)
				&& config_setting_lookup_int(mapping, "ir_address", &address)
				&& config_setting_lookup_int(mapping, "ir_command", &command)) ) {
				continue;
			}

			// if the IR data doesn't match, proceed to next mapping
			if (protocol != ir_code->protocol ||
				address != ir_code->address ||
				command != ir_code->command) {
				continue;
			}

			// send key (sequence) if there is any and this feature is enabled
			if (send_keys
				&& config_setting_lookup_string(mapping, "key", &key)) {
				xdo_send_keysequence_window(x, CURRENTWINDOW, key, 2000);
			}

			// start app if there is any and this feature is enabled
			if (start_apps
				&& config_setting_lookup_string(mapping, "application", &application)
				&& config_setting_lookup_string(mapping, "parameter", &parameter)) {
				char call[256];
				strcpy(call, application);
				strcat(call, " ");
				strcat(call, parameter);
				system(call);
			}
		}
	}
}


void cleanup(void) {
	int res;

	// close xdo
	xdo_free(x);

	// close the config
	config_destroy(&cfg);

	// close the device
	hid_close(handle);

	// finalize the hidapi library
	res = hid_exit();
	if (res != 0) {
		fprintf(stderr, "hid_exit() failed. Errorcode: %d\n", res);
	}
}


int main(int argc, char* argv[]) {
	int res;
	char option;
	bool verbose = false;

	// prepare xdo and config
	x = xdo_new(NULL);
	config_init(&cfg);

	// try to read config file or create one if none exists
	if (access(config_file, R_OK) == 0) {
		// file exists
		// read the file. if there is an error, report it and exit
		if (!config_read_file(&cfg, config_file)) {
			fprintf(stderr, "Error reading config file %s, line %d - %s\n",
					config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
			config_destroy(&cfg);
			exit(EXIT_FAILURE);
		}
	}
	else {
		// file doesn't exist
		create_config_file();
	}

	// register cleanup function
	res = atexit(cleanup);
	if (res != 0) {
		fprintf(stderr, "Registering cleanup failed. Errorcode: %d\n", res);
		exit(EXIT_FAILURE);
	}

	// initialize the hidapi library
	res = hid_init();
	if (res != 0) {
		fprintf(stderr, "hid_init() failed. Errorcode: %d\n", res);
		exit(EXIT_FAILURE);
	}

	// open the device using the VID and PID
	handle = hid_open(HIDIRT_VID, HIDIRT_PID, NULL);
	if (handle == NULL) {
		fprintf(stderr, "hid_open() failed. Maybe device is not connected.\n");
		exit(EXIT_FAILURE);
	}

	// set the hid_read() function to be non-blocking
	res = hid_set_nonblocking(handle, 1);
	if (res != 0) {
		fprintf(stderr, "hid_set_nonblocking() failed. Errorcode: %d\n", res);
		exit(EXIT_FAILURE);
	}

	// handle the received program arguments
	while ((option = getopt(argc, argv, "b::i::n::f::r::m::t::d::w::s::u::e::a::x::v")) != -1) {
		switch (option) {
			case 'b': // control the buttons
				feature_bool(handle, ControlPcEnable, optarg);
				break;

			case 'i': // forward IR codes
				feature_bool(handle, ForwardIrEnable, optarg);
				break;

			case 'n': // power on IR code
				feature_ircode(handle, PowerOnCode, optarg);
				break;

			case 'f': // power off IR code
				feature_ircode(handle, PowerOffCode, optarg);
				break;

			case 'r': // reset IR code
				feature_ircode(handle, ResetCode, optarg);
				break;

			case 'm': // minimum repeats
				feature_bool(handle, MinRepeats, optarg);
				break;

			case 't': // date/time
				feature_devicetime(handle, DeviceTime, optarg);
				break;

			case 'd': // clock deviation
				feature_timedeviation(handle, ForwardIrEnable, optarg);
				break;

			case 'w': // wakeup time
				feature_waketime(handle, WakeupTime, optarg);
				break;

			case 's': // wakeup time span
				feature_bool(handle, WakeupTimeSpan, optarg);
				break;

			case 'u': // start firmware update mode by writing 0x5a as first data byte
				feature_bool(handle, RequestBootloader, optarg);
				break;

			case 'e': // enable watchdog
				feature_bool(handle, WatchdogEnable, optarg);
				break;

			case 'a': // service watchdog
				feature_bool(handle, WatchdogReset, optarg);
				break;

			case 'x': // transmit custom code
				send_ircode(handle, optarg);
				break;

			case 'v': // verbose mode
				verbose = true;
				break;

			case '?': // help
				fprintf(stdout, "Some help text missing.\n");
				break;

			default:
				fprintf(stderr, "Unknown option: %c\n", option);
				exit(EXIT_FAILURE);
		} // switch (option)
	} // while ((option = getopt(argc, argv, "...")) != -1)

	if (verbose == true) {
		show_device_details(handle);
	} // if (verbose == true)

	while ((argc <= 1) || (verbose == true)) {
		unsigned char buf[MAX_FEATURE_REPORT_LENGTH];
		struct ircode ir_code;

		usleep(25*1000); // USB polling interval is 25ms
		res = hid_read(handle, buf, sizeof(buf));

		if (res < 0) {
			fprintf(stderr, "hid_read() failed. Maybe device was disconnected. Errorcode: %d\n", res);
			fprintf(stderr, "Trying to reconnect.\n");

			// close the device
			hid_close(handle);

			// try to reconnect
			while (res != 0) {
				usleep(500*1000);

				// reopen the device using the VID and PID
				handle = hid_open(HIDIRT_VID, HIDIRT_PID, NULL);
				if (handle != NULL) {
					// set the hid_read() function to be non-blocking
					res = hid_set_nonblocking(handle, 1);
				}
			}
		}
		else if (res > 0) {
			if (buf[0] == IrCodeInterrupt) {
				// prepare and print result
				ir_code.protocol = buf[1];
				ir_code.address = buf[3];
				ir_code.address <<= 8;
				ir_code.address += buf[2];
				ir_code.command = buf[5];
				ir_code.command <<= 8;
				ir_code.command += buf[4];
				ir_code.flags = buf[6];
				if (verbose == true) {
					fprintf(stdout, "0x%02hhx,0x%04hx,0x%04hx,0x%02hhx\n",
							ir_code.protocol, ir_code.address, ir_code.command, ir_code.flags);
				}

				// handle the received IR code
				handle_ir_code(&ir_code);
			}
			else {
				fprintf(stderr, "Unknown ReportID: %d.\n", buf[0]);
			}
		} // if (res > 0)
	} // while ((argc <= 1) || (verbose == true))

	return EXIT_SUCCESS;
}
