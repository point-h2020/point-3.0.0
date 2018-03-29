/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

#include <blackadder.hpp>
#include <signal.h>

Blackadder *ba;
int payload_size = 1000;
char *payload = (char *) malloc(payload_size);

void sigfun(int sig) {
    (void) signal(SIGINT, SIG_DFL);
    ba->disconnect();
    free(payload);
    delete ba;
    exit(0);
}

int main(int argc, char* argv[]) {
    (void) signal(SIGINT, sigfun);
    if (argc > 1) {
        int user_or_kernel = atoi(argv[1]);
        if (user_or_kernel == 0) {
            ba = Blackadder::Instance(true);
        } else {
            ba = Blackadder::Instance(false);
        }
    } else {
        /*By Default I assume blackadder is running in user space*/
        ba = Blackadder::Instance(true);
    }
    cout << "Process ID: " << getpid() << endl;
    string id = "0000000000000000";
    string prefix_id;
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);

    prefix_id = "0000000000000000";
    id = "0111111111111111";
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    while (true) {
      Event ev;
      ba->getEvent(ev);
      switch (ev.type) {
      case SCOPE_PUBLISHED:
        cout << "SCOPE_PUBLISHED: " << chararray_to_hex(ev.id) << endl;
        break;
      case SCOPE_UNPUBLISHED:
        cout << "SCOPE_UNPUBLISHED: " << chararray_to_hex(ev.id) << endl;
        break;
      case START_PUBLISH:
        cout << "START_PUBLISH: " << chararray_to_hex(ev.id) << endl;
	ba->publish_data(ev.id, DOMAIN_LOCAL, NULL, 0, payload,
			 payload_size);
        break;
     case STOP_PUBLISH:
        cout << "STOP_PUBLISH: " << chararray_to_hex(ev.id) << endl;
        break;
      case PUBLISHED_DATA:
        cout << "PUBLISHED_DATA: " << chararray_to_hex(ev.id) << endl;
        break;
      default:
        cout<<"unknown event\n";
      }
    }
    sleep(5);
    free(payload);
    ba->disconnect();
    delete ba;
    return 0;
}

