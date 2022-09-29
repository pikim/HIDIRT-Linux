# HIDIRT-Linux
Linux host application for HIDIRT (HID InfraRed Transceiver) written in C.

## Configuration

### Section 'settings'
    send_keys = true|false;
        Enable or disable mapping IR codes to keypresses.

    start_apps = true|false;
        Enable or disable starting an application (with arguments).

    sync_clocks = true|false;  => not implemented, yet
        Enable or disable clock synchronization between host and USB device.

    pc_clock_is_origin = true|false;  => not implemented, yet
        Defines if the host clock is the origin. If true the host clock is copied to the device; if false the device clock is copied to the host.

    calibration_start_time = 0L;  => not implemented, yet
        Helps to calibrate the device clock. Do not modify manually.


### Section 'mappings'
    {
        description = "any helpful name";
            Name for the mapping. Isn't used by the application and therefore it's only useful for documentation purposes.

        ir_protocol = 0x02;
        ir_address = 0x5aa5;
        ir_command = 0x000a;

        key = "A";
            Keysequence to be sent. Any combination of X11 KeySym names separated by '+' are valid. Single KeySym names are valid, too. KeySym names can be found using 'xorg-xev' tool.

        application = "/path/to/binary";
            Application to be started. Ideally this is an absolute path to a binary or shell script. The daemon waits until the application finishes. So, if you want to start a long-running process, you must use an appropriate shell script as starter.

        parameter = "# arg1 arg2";
            Parameters for the application.
    },

## Usage
hidirt [options]

## Options
    no option
      Starts the binary in daemon mode that waits for IR codes and eventually maps them to key presses and/or starts a predefined application (with predefined arguments).

    -b[=0|1]
      Read state, enable or disable controlling the buttons. When enabled, the hardware device controls the power and reset buttons.

    -i[=0|1]
      Read state, enable or disable forwarding of IR codes. When enabled, the hardware device forwards received IR codes using the IR transmission diode.

    -n[=protocol,address,command,flags]
      Read or set the power on IR code. When this IR code is received, the hardware device controls the power button when the PC is off.

    -f[=protocol,address,command,flags]
      Read or set the power off IR code. When this IR code is received, the hardware device controls the power button when the PC is on.

    -r[=protocol,address,command,flags]
      Read or set the reset IR code. When this IR code is received, the hardware device controls the reset button.

    -m[=0 to 255]
      Read or set the minimum repeats number. Kind of debounces IR codes so that only every n-th code repetition is generating an interrupt.

    -t[=sec.msec]
      Read or set the device date/time. Together with the wakeup time, this allows to turn the PC on at a given time. sec is uint32, msec is int16.

    -d[=-2,147,483,648 to 2,147,483,647]
      Read or set the clock deviation. This allows to correct an eventual clock deviation on the hardware device. Depending on whether the clock runs too fast or too slow, the value must be negative or positive.

    -w[=0 to 4,294,967,295]
      Read or set the wakeup date/time. Together with the device time, this allows to turn the PC on at a given time.

    -s[=0 to 255]
      Read or set the wakeup time span. If the exact wakeup time was missed due to a power fail, this value defines the tolerable wakeup time delay in minutes.

    -u[=0|1]  => not properly documented, yet
      Start firmware update mode by writing 0x5a as first data byte.

    -e[=0|1]
      Read state, enable or disable watchdog. If enabled, the watchdog must be serviced every 2 seconds using the option below.

    -a[=0|1]
      If the watchdog is enabled, it must be serviced every 2 seconds by passing 1 to this option. This avoids a reset of the hardware device.

    -x=protocol,address,command,flags
      Transmit custom IR code using the IR transmission diode.

    -v
      Verbose mode. Prints some device informations and then waits for IR codes as if the binary was started without any option, see "no option" above.
