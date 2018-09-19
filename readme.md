setultrastik360 is a command line utility to set the status of the [Ultimarc UltraStik360](http://ultimarc.com/ultrastik_info.html)

# dependencies
- libusb-1.0
- cmake 3.5.0 (3.8.0 recommended)

# caveats

I have no test hardware at my disposal. As such the program flow has not been completely tested.
The methodology has been confirmed working though so logically this project has to behave as intended.

# compilation:

    cd 'folder you extracted the archive to'
    mkdir -p builds/unix
    cd builds/unix
    cmake ../..
    make
    make install (as super user)