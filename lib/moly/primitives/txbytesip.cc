/*
 * txbyteshttp.cc
 *
 *  Created on: 29 Sep 2017
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the MOnitoring LibrarY (MOLY).
 *
 * MOLY is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOLY is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOLY. If not, see <http://www.gnu.org/licenses/>.
 */

#include "txbytesip.hh"

TxBytesIp::TxBytesIp(node_identifier_t nodeId, bytes_t txBytes)
	: _nodeId(nodeId),
	  _txBytes(txBytes)
{
	_size = sizeof(node_identifier_t) + sizeof(bytes_t);
	_pointer = (uint8_t *)malloc(_size);
	_composePacket();
}

TxBytesIp::TxBytesIp(uint8_t *pointer, uint32_t size)
{
	_pointer = (uint8_t *)malloc(size);
	memcpy(_pointer, pointer, size);
	_decomposePacket();
}

TxBytesIp::~TxBytesIp()
{
	free(_pointer);
}

node_identifier_t TxBytesIp::nodeId()
{
	return _nodeId;
}

uint8_t * TxBytesIp::pointer()
{
	return _pointer;
}

string TxBytesIp::primitiveName()
{
	return "TX_BYTES_IP_M";
}

string TxBytesIp::print()
{
	ostringstream oss;
	oss << " | Node ID: " << nodeId();
	oss << " | TX Bytes: " << txBytes();
	return oss.str();
}

size_t TxBytesIp::size()
{
	return _size;
}

bytes_t TxBytesIp::txBytes()
{
	return _txBytes;
}

void TxBytesIp::_composePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(bufferPointer, &_nodeId, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] RX Bytes
	memcpy(bufferPointer, &_txBytes, sizeof(bytes_t));
}

void TxBytesIp::_decomposePacket()
{
	uint8_t * bufferPointer = _pointer;
	// [1] Node ID
	memcpy(&_nodeId, bufferPointer, sizeof(node_identifier_t));
	bufferPointer += sizeof(node_identifier_t);
	// [2] TX Bytes
	memcpy(&_txBytes, bufferPointer, sizeof(bytes_t));
}
