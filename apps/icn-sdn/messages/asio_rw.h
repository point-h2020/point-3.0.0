/*
 * AsioAdapting.h
 *
 *  Created on: Oct 21, 2016
 *      Author: geopet
 */

#ifndef ASIORW_H_
#define ASIORW_H_
#pragma once
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <boost/asio.hpp>

using namespace google::protobuf::io;


template <typename SyncReadStream>
class AsioInputStream : public CopyingInputStream {
public:
    AsioInputStream(SyncReadStream& sock);
    int Read(void* buffer, int size);
private:
    SyncReadStream& m_Socket;
};


template <typename SyncReadStream>
AsioInputStream<SyncReadStream>::AsioInputStream(SyncReadStream& sock) :
    m_Socket(sock) {}


template <typename SyncReadStream>
int
AsioInputStream<SyncReadStream>::Read(void* buffer, int size)
{
    std::size_t bytes_read;
    boost::system::error_code ec;
    bytes_read = m_Socket.read_some(boost::asio::buffer(buffer, size), ec);

    if (!ec) {
        return bytes_read;
    }
    else if (ec == boost::asio::error::eof) {
        return 0;
    }
    else {
        return -1;
    }
}


template <typename SyncWriteStream>
class AsioOutputStream : public CopyingOutputStream {
public:
    AsioOutputStream(SyncWriteStream& sock);
    bool Write(const void* buffer, int size);
private:
    SyncWriteStream& m_Socket;
};


template <typename SyncWriteStream>
AsioOutputStream<SyncWriteStream>::AsioOutputStream(SyncWriteStream& sock) :
    m_Socket(sock) {}


template <typename SyncWriteStream>
bool
AsioOutputStream<SyncWriteStream>::Write(const void* buffer, int size)
{
    boost::system::error_code ec;
    m_Socket.write_some(boost::asio::buffer(buffer, size), ec);
    return !ec;
}




#endif /* ASIORW_H_ */
