# About
BlAckadder Monitoring wraPpER clasS (BAMPERS) is a shared library written in C++
which allows any other C/C++ application to utilise an ICN pub/sub sematic to
push data to the monitoring server without writing a single line of ICN
(Blackadder) related code.

Why BAMPERS: Combination of "business" and "Pampers," describing diapers for
business people - used when time is money and even a trip to the bathroom will
slow you down.

# Author
Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
InterDigital Europe, Ltd.

# Compilation
BAMPERS can only be built on Linux systems; more precisely, Debian-based
distributions. If RedHat or FreeBSD-based distributions are used simply adjust
the make file to adopt the locations of shared lirbaries on your system how to
link them once being used.

For how to compile and install BAMPERS please read the INSTALL file located in
the same directory.

# Documentation
BAMPERS has Doxygen-friendly source code and comes with a pre-configured Doxygen
configuration file within the /doc directory. In order to generate the HTML code
for a more user-friendly source code documentation simply run the following
commands:

~$ sudo apt-get install graphviz doxygen
~$ cd lib/bampers/doc
~$ doxygen doxygen.conf

Now point your preferred web browser to lib/bampers/doc/html/index.html.

# License
BAMPERS is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
BAMPERS. If not, see <http://www.gnu.org/licenses/>,
