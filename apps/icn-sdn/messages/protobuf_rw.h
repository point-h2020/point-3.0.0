/*
 * Copyright (C) 2016  George Petropoulos
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef PROTOBUFRW_H_
#define PROTOBUFRW_H_

#pragma once
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message_lite.h>
namespace google {
    namespace protobuf {
        namespace io {
            bool writeDelimitedTo(
                const google::protobuf::MessageLite& message,
                google::protobuf::io::ZeroCopyOutputStream* rawOutput);

            bool readDelimitedFrom(
                google::protobuf::io::ZeroCopyInputStream* rawInput,
                google::protobuf::MessageLite* message);
        }
    }
}




#endif /* PROTOBUFRW_H_ */
