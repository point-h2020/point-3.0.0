# Author
Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>, InterDigital Europe, Ltd.

# Installation

In order to make the NAP ensure you have installed all required libraries listed in apt-get.txt in the root directory of this git clone and run

$ make
$ sudo make install

In order to run the NAP the required configuration files must be available in /etc/nap. Example files have been copied to /usr/share/doc/nap. Simply copy them over:

$ cp /usr/share/doc/nap /etc/nap

# Configuration
Based on the number of endpoints and HTTP sessions the NAP is supposed to handle
threading, TCP and generic file descriptor settings must be adjusted in the
kernel. Recommended values for NAPs are

Threads:
echo "100000" > /proc/sys/kernel/threads-max

# Documentation
## General Documentation
The NAP comes with a comprehensive documentation which is supposed to bridge the gap between the system-level description in POINT deliverables and the doxygen code documentation.

In order to compile the tex document into a PDF, install texlive 

$ apt install texlive-full

and make the documentation in doc/tex by invoking

$ make

## Code
The NAP is entirely written in C++ and follows Doxygen syntax conventions to document the code. In order to generate the HTML files invoke

$ doxygen doxygen.conf

with the /doc directory which generates the HTML files within a dedicated folder. Make sure that graphviz has been installed beforehand:

$ apt install graphviz
