#Tarantula Scheduling Engine


Tarantula is a tool for live TV broadcast scheduling, designed to control and
automate various broadcast equipment.

Tarantula is free software, licensed under the GNU General Public License.
See the file COPYING for copying conditions.

###Downloading

Tarantula can be cloned using git  and the following command:

    git clone https://github.com/YSTV/Tarantula


###Building

Build Tarantula by issuing the `make` command inside the project directory.
Tarantula requires gcc-4.7 or above.

###Using Tarantula

The default configuration enables several demo plugins and provides a simple
user interface to add schedule events to the system. Copy the configuration 
files from config_base/ to config/ and edit as needed to enable other plugins.

Run `bin/Tarantula` after the build has completed to start the system. Then
visit http://localhost:9816/ in a browser to access the web user interface.
Events added in the web interface will be executed and the results will 
appear in the log (demo plugins do nothing but write to the log).


###Extending Tarantula

Tarantula is free software, anybody is welcome to extend it, for example by
writing plugins to complement the default set. To keep track of development
and contribute, email tarantula@ystv.co.uk.


