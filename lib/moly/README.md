# About
MOnitoring LibrarY (MOLY) is an application interface which allwos any
application to report monitoring data to a monitoring agent. The agent then uses
BAMPERS to publish the monitoring data to a monitoring server using 

# Authors
Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>, InterDigital Europe,
Ltd.

Mays Al-Naday <mfhaln@essex.ac.uk>, University of Essex

# Compliation
For detailed instructions on how to build MOLY please read the INSTALL file.

#Documentation
MOLY has Doxygen-friendly source code and comes with a pre-configured Doxygen
configuration file within the /doc directory. In order to generate the HTML code
for a more user-friendly source code documentation simply run the following
commands:

~$ sudo apt-get install graphviz doxygen

~$ cd lib/moly/doc

~$ doxygen doxygen.conf

Now point your preferred web browser to lib/moly/doc/html/index.html.

# License
This file is part of the MOnitoring LibrarY (MOLY).

MOLY is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

MOLY is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MOLY. If not, see <http://www.gnu.org/licenses/>.