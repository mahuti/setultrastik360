setultrastik360 is a command line utility to set the status of the 2nd generation [Ultimarc UltraStik360](http://ultimarc.com/ultrastik_info.html)

(The 1st generation is a completely different device and was obsoleted by its creator years ago)

# dependencies
- libusb-1.0
- cmake 3.5.0 (3.8.0 recommended)

# caveats

In the absence of test hardware at coding time it was not possible to completely test the program.
The methodology it uses at hardware level has been confirmed previously so logically is has to behave as intended.
Of course, one can never be sure until an end to end test of all options has been conducted.

# compilation:

    cd 'folder you extracted the archive to'
    mkdir -p builds/unix
    cd builds/unix
    cmake ../..
    make
    make install (as super user)