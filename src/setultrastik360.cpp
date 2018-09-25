/*
    setultrastik360 - rotates Ultimarc's UltraStick 360 joystick to any
    of the vendor supplied supported maps with physical restrictor support.
    Copyright (C) 2018 De Waegeneer Gijsbrecht

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    contact: gijsbrecht.dewaegeneer@telenet.be
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <libusb-1.0/libusb.h>
#include "u360maps.h"

#define UM_REQUEST_TYPE 0x21
#define UM_REQUEST 9
#define UM_TIMEOUT 2000
#define U360_VENDOR 0xD209
#define U360_PRODUCT 0x0511
#define U360_VALUE 0x0200
#define U360_MESG_LENGTH 4
#define U360_INTERFACE 2
#define U360_WRITE_CYCLES 24
#define U360_HARDWARE_WRITE_DELAY 417
#define KERNEL_DRIVER_ATTACHED 1
#define VERSION "1.0.0"

auto getdevice(libusb_context *&context, const unsigned int &vendor, const unsigned int &product, std::vector<std::tuple<libusb_device *, int, int>> &devicelist) {
    libusb_device **devices;
    libusb_device *device(nullptr);
    for (auto idx(0); idx < libusb_get_device_list(context, &devices); idx++) {
        device = devices[idx];
        libusb_device_descriptor descriptor = {0};
        auto rc = libusb_get_device_descriptor(device, &descriptor);
        if (rc != LIBUSB_SUCCESS) {
            std::cout << "WARNING: " << libusb_error_name(rc) << " - " << libusb_strerror((libusb_error) rc) << " - trying to proceed...\n";
        } else {
            if ((descriptor.idVendor == vendor) && ((descriptor.idProduct >= product) && (descriptor.idProduct <= product + 3))) {
                devicelist.emplace_back(std::make_tuple(device, descriptor.idVendor, descriptor.idProduct));
            }
        }
    }
    libusb_free_device_list(devices, 1);
    return !devicelist.empty();
}

void cleanup(libusb_context *&context, libusb_device_handle *&devicehandle) {
    if (devicehandle) { libusb_close(devicehandle); }
    if (context) { libusb_exit(context); }
}

void errorhandler(libusb_context *&context, libusb_device_handle *&devicehandle, const std::string &errormessage) {
    std::cerr << "ERROR: " << errormessage << "\n";
    cleanup(context, devicehandle);
    exit(EXIT_FAILURE);
}

void errorhandler(libusb_context *&context, libusb_device_handle *&devicehandle, int rc) {
    std::cerr << "ERROR: " << libusb_error_name(rc) << " - " << libusb_strerror((libusb_error) rc) << "\n";
    cleanup(context, devicehandle);
    exit(EXIT_FAILURE);
}

auto applyU360map(long int mapId, bool hasRestrictor) {
    libusb_context *context(nullptr);
    std::vector<std::tuple<libusb_device *, int, int>> devicelist;
    libusb_device_handle *devicehandle(nullptr);
    auto rc(0);

    rc = libusb_init(&context);
    if (rc != LIBUSB_SUCCESS) { errorhandler(context, devicehandle, rc); }
    libusb_set_debug(context, LIBUSB_LOG_LEVEL_INFO);

    if (!getdevice(context, U360_VENDOR, U360_PRODUCT, devicelist)) {
        errorhandler(context, devicehandle, "No UltraStik360 devices were found.");
    } else { //good to go
        populateU360BehavioralMaps();
        for (auto &device : devicelist) {
            rc = libusb_open(std::get<0>(device), &devicehandle);
            if (rc != LIBUSB_SUCCESS) { errorhandler(context, devicehandle, rc); }
            if (devicehandle) {
                if (libusb_kernel_driver_active(devicehandle, U360_INTERFACE) == KERNEL_DRIVER_ATTACHED) {
                    rc = libusb_detach_kernel_driver(devicehandle, U360_INTERFACE);
                    if (rc != LIBUSB_SUCCESS) { errorhandler(context, devicehandle, rc); }
                }
                rc = libusb_claim_interface(devicehandle, U360_INTERFACE);
                if (rc != LIBUSB_SUCCESS) { errorhandler(context, devicehandle, rc); }

                std::get<1>(u360BehavioralMaps[mapId])[2] = hasRestrictor ? (unsigned char) 0x10 : (unsigned char) 0x09; //restrictor_on : restrictor_off

                //TODO begin untested code segment - no test hardware - needs end2end testing
                rc = 0; //U360 requires 24 writes of 4 bytes - reset rc
                for (size_t i(0); i < (U360_MESG_LENGTH * U360_WRITE_CYCLES); i += U360_MESG_LENGTH) {
                    std::slice slc(i, U360_MESG_LENGTH, 1);
                    std::valarray<unsigned char> writeCycleSlice = std::get<1>(u360BehavioralMaps[mapId])[slc];
                    rc += libusb_control_transfer(devicehandle, UM_REQUEST_TYPE, UM_REQUEST, U360_VALUE, U360_INTERFACE, &writeCycleSlice[0], U360_MESG_LENGTH, UM_TIMEOUT);
                    std::this_thread::sleep_for(std::chrono::microseconds(U360_HARDWARE_WRITE_DELAY));
                }
                std::stringstream ss;
                ss << std::hex << "U360 0x" << std::get<1>(device) << ":0x" << std::hex << std::get<2>(device) << " (Restrictor:" << (hasRestrictor ? "On" : "Off") << ")"
                   << std::get<0>(u360BehavioralMaps[mapId]) << " -> " << ((rc == U360_MESG_LENGTH * U360_WRITE_CYCLES) ? "SUCCESS" : "FAILURE") << "\n";
                std::cout << ss.str();
                //TODO end untested code segment - no test hardware - needs end2end testing

                rc = libusb_release_interface(devicehandle, U360_INTERFACE);
                if (rc != LIBUSB_SUCCESS) { errorhandler(context, devicehandle, rc); }
            }
        }
    }
    cleanup(context, devicehandle);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    long int mapId(0);
    bool hasRestrictor(false);
    switch (argc) {
        case 3:
            if (argv[2] == "-r") { hasRestrictor = true; }
        case 2:
            try {
                mapId = std::stol(argv[1], nullptr, 10);
                if (mapId < 1 || mapId > 9) { mapId = 0; }
            }
            catch (std::exception &) { mapId = 0; }
            if (!mapId) {
                std::cerr << "Wrong arguments (allowed values [1-9])\n";
                return EXIT_FAILURE;
            }
            break;
        default:
            std::cout
                    << " _____     _   _____ _ _           _____ _   _ _   ___ ___ ___ \n"
                    << "|   __|___| |_|  |  | | |_ ___ ___|   __| |_|_| |_|_  |  _|   |\n"
                    << "|__   | -_|  _|  |  | |  _|  _| .'|__   |  _| | '_|_  | . | | |\n"
                    << "|_____|___|_| |_____|_|_| |_| |__,|_____|_| |_|_,_|___|___|___|\n"
                    << "setultrastik360 Copyright (C) 2018  De Waegeneer Gijsbrecht\n"
                    << "Ultimarc UltraStik360 switcher Version " << VERSION << "\n\n"
                    << "[ " << argv[0] << " map (-r) ] apply map x to all U360's , x being:\n"
                    << "x  map name\n"
                    << "1  2-Way, Left & Right\n"
                    << "2  2-Way, Up & Down\n"
                    << "3  4-Way, Diagonals Only\n"
                    << "4  4-Way, No Sticky (UD Bias)\n"
                    << "5  4-Way\n"
                    << "6  8-Way Easy Diagonals\n"
                    << "7  8-Way\n"
                    << "8  Analog\n"
                    << "9  Mouse Pointer\n"
                    << "optionally add -r to activate restrictor support.\n\n"
                    << "This program comes with ABSOLUTELY NO WARRANTY. This is free software,\nand you are welcome to redistribute it under certain conditions.\n"
                    << "license: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007\nCopyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>\n";
            return EXIT_SUCCESS;
    }
    return applyU360map(mapId, hasRestrictor);
}
