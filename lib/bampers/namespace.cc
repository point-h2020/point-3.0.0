/*
 * namespace.cc
 *
 *  Created on: Dec 4, 2015
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of BlAckadder Monitoring wraPpER clasS (BAMPERS).
 *
 * BAMPERS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAMPERS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAMPERS. If not, see <http://www.gnu.org/licenses/>.
 */

#include "namespace.hh"

Namespace::Namespace(Blackadder *blackadder, uint32_t nodeId)
	: _blackadder(blackadder),
	  _localNodeId(nodeId)
{}

Namespace::~Namespace() {}

void Namespace::addPublishedScopePath(IcnId *icnId)
{
	_mutexPublishedScopePaths.lock();
	_publishedScopePaths.insert((pair<string, bool>(icnId->str(), false)));
	_mutexPublishedScopePaths.unlock();
}

void Namespace::advertiseInformationItem(IcnId *icnId)
{
	_blackadder->publish_info(icnId->binId(), icnId->binPrefixId(),
			DOMAIN_LOCAL, NULL, 0);
}

Blackadder * Namespace::getBlackadderInstance()
{
	return _blackadder;
}

bool Namespace::getDataFromBuffer(IcnId *icnId, uint8_t *data,
		uint32_t &dataSize)
{
	_mutexBuffer.lock();
	_itBuffer = _buffer.find(icnId->str());

	if (_itBuffer == _buffer.end())
	{
		_mutexBuffer.unlock();
		return false;
	}

	dataSize = _itBuffer->second.top().dataSize;
	memcpy(data, _itBuffer->second.top().data, dataSize);
#ifdef BAMPERS_DEBUG
	cout << "((BAMPERS)) "
			<< "Obtained " << dataSize << " bytes from ICN buffer. "
			<< _itBuffer->second.size() << " items left in stack\n";
#endif
	free(_itBuffer->second.top().data);
	_itBuffer->second.pop();

	//remove entire map key if stack is empty
	if (_itBuffer->second.empty())
	{
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "ICN buffer stack is empty for CID " << icnId->str()
				<< ". Deleting entire map key\n";
#endif
		_buffer.erase(icnId->str());
	}

	_mutexBuffer.unlock();
	return true;
}

string Namespace::getScopePath(node_role_t nodeRole,
		bampers_info_item_t infoItem)
{
	ostringstream oss;
	oss << setw(ID_LEN) << setfill('0') << (int)NAMESPACE_MONITORING;
	oss << setw(ID_LEN) << setfill('0') << (int)TOPOLOGY_NODES;
	oss << setw(ID_LEN) << setfill('0') << (int)nodeRole;
	oss << setw(ID_LEN) << setfill('0') << _localNodeId;
	oss << setw(ID_LEN) << setfill('0') << (int)infoItem;
	return oss.str();
}

string Namespace::getScopePath(uint32_t nodeId, uint16_t portId,
		bampers_info_item_t infoItem)
{
	ostringstream oss;
	oss << setw(ID_LEN) << setfill('0') << (int)NAMESPACE_MONITORING;
	oss << setw(ID_LEN) << setfill('0') << (int)TOPOLOGY_PORTS;
	oss << setw(ID_LEN) << setfill('0') << nodeId;
	oss << setw(ID_LEN) << setfill('0') << portId;
	oss << setw(ID_LEN) << setfill('0') << (int)infoItem;
	return oss.str();
}

string Namespace::getScopePath(node_role_t nodeRole, uint32_t nodeId,
		bampers_info_item_t infoItem)
{
	ostringstream oss;
	oss << setw(ID_LEN) << setfill('0') << (int)NAMESPACE_MONITORING;
	oss << setw(ID_LEN) << setfill('0') << (int)TOPOLOGY_NODES;
	oss << setw(ID_LEN) << setfill('0') << (int)nodeRole;

	switch (infoItem)
	{
	case II_CPU_UTILISATION:
		if (nodeRole == NODE_ROLE_UE || nodeRole == NODE_ROLE_SERVER)
		{
			oss << setw(ID_LEN) << setfill('0') << _localNodeId;
			oss << setw(ID_LEN) << setfill('0') << nodeId;//== endpointId
		}
		else
		{
			oss << setw(ID_LEN) << setfill('0') << nodeId;
		}
		break;
	case II_END_TO_END_LATENCY:// /nid/endpointId
		oss << setw(ID_LEN) << setfill('0') << _localNodeId;
		oss << setw(ID_LEN) << setfill('0') << nodeId;//== endpointId
		break;
	default://NID only
		oss << setw(ID_LEN) << setfill('0') << nodeId;//== endpointId
	}

	oss << setw(ID_LEN) << setfill('0') << (int)infoItem;
	return oss.str();
}

string Namespace::getScopePath(uint16_t linkId, uint32_t destinationNodeId,
		uint32_t sourceNodeId, uint16_t sourcePort, link_type_t linkType,
		bampers_info_item_t infoItem)
{
	ostringstream oss;
	oss << setw(ID_LEN) << setfill('0') << (int)NAMESPACE_MONITORING;
	oss << setw(ID_LEN) << setfill('0') << (int)TOPOLOGY_LINKS;
	oss << setw(ID_LEN) << setfill('0') << linkId;
	oss << setw(ID_LEN) << setfill('0') << destinationNodeId;
	oss << setw(ID_LEN) << setfill('0') << sourceNodeId;
	oss << setw(ID_LEN) << setfill('0') << sourcePort;
	oss << setw(ID_LEN) << setfill('0') << (int)linkType;
	oss << setw(ID_LEN) << setfill('0') << (int)infoItem;
	return oss.str();
}

void Namespace::prepareDataToBePublished(string scopePath, uint8_t *data,
				uint32_t dataLength)
{
	IcnId icnId;
	icnId = scopePath;
	prepareDataToBePublished(&icnId, data, dataLength);
}

void Namespace::prepareDataToBePublished(IcnId *icnId, uint8_t *data,
		uint32_t dataLength)
{
	// Scope not published
	if (!publishedScopePath(icnId))
	{
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "Scope path " << icnId->print() << " has not been published\n";
#endif
		_mutexPublishedScopePaths.lock();
		_addDataToBuffer(icnId, data, dataLength);
		_publishScopePath(icnId);
		_mutexPublishedScopePaths.unlock();
		return;
	}

	// No subscriber available
	if (!subscriberAvailable(icnId))
	{
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) No subscriber available for " << icnId->print()
				<< " available\n";
#endif
		_addDataToBuffer(icnId, data, dataLength);
		advertiseInformationItem(icnId);
		return;
	}

	// Scope published, subscribers available. Go off and publish it now
	_blackadder->publish_data(icnId->binIcnId(), DOMAIN_LOCAL, NULL,
			0, data, dataLength);
#ifdef BAMPERS_DEBUG
	cout << "((BAMPERS)) "
			<< "Data of length " << dataLength << " published under scope path "
			<< icnId->print() << endl;
#endif
}

bool Namespace::publishedScopePath(IcnId *icnId)
{
	_mutexPublishedScopePaths.lock();
	_itPublishedScopePaths = _publishedScopePaths.find(icnId->str());

	if (_itPublishedScopePaths == _publishedScopePaths.end())
	{
		_mutexPublishedScopePaths.unlock();
		return false;
	}

	_mutexPublishedScopePaths.unlock();
	return true;
}

void Namespace::removePublishedScopePath(IcnId *icnId)
{

	if (!publishedScopePath(icnId))
	{
		return;
	}

	_mutexPublishedScopePaths.lock();
	_publishedScopePaths.erase(icnId->str());
	_mutexPublishedScopePaths.unlock();
}

void Namespace::setSubscriberAvailability(IcnId *icnId, bool state)
{
	if (!publishedScopePath(icnId))
	{
		return;
	}

	_mutexPublishedScopePaths.lock();
	_itPublishedScopePaths = _publishedScopePaths.find(icnId->str());
	(*_itPublishedScopePaths).second = state;
	_mutexPublishedScopePaths.unlock();
#ifdef BAMPERS_DEBUG
	cout << "((BAMPERS)) "
			<< "Subscriber availability for scope path " << icnId->print()
			<< " set to " << state << endl;
#endif
}

bool Namespace::subscriberAvailable(IcnId *icnId)
{
	if (!publishedScopePath(icnId))
	{
		return false;
	}

	_mutexPublishedScopePaths.lock();
	_itPublishedScopePaths = _publishedScopePaths.find(icnId->str());

	// Subscriber available
	if ((*_itPublishedScopePaths).second)
	{
		_mutexPublishedScopePaths.unlock();
		return true;
	}

	// No subscriber available
	_mutexPublishedScopePaths.unlock();
	return false;
}

void Namespace::_addDataToBuffer(IcnId *icnId, uint8_t *data,
		uint32_t dataSize)
{
	if (icnId->str().length() == 0)
	{
#ifdef BAMPERS_DEBUG
		cout << "Scope path is empty!\n";
#endif
		return;
	}

	_mutexBuffer.lock();
	_itBuffer = _buffer.find(icnId->str());
	data_t itemStruct;
	itemStruct.data = (uint8_t *)malloc(dataSize);
	memcpy(itemStruct.data, data, dataSize);
	itemStruct.dataSize = dataSize;

	// Add new entry
	if (_itBuffer == _buffer.end())
	{
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) New data will be added to buffer : "
				<< icnId->print() << endl;
#endif
		stack<data_t> itemStack;
		itemStack.push(itemStruct);
		_buffer.insert(pair<string, stack<data_t>>(icnId->str(), itemStack));
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "New map key created in ICN buffered for CID "
				<< icnId->print() << " and packet of length " << dataSize
				<< " has been buffered" << endl;
#endif
	}
	// Add data to stack
	else
	{
#ifdef BAMPERS_DEBUG
		cout << "((BAMPERS)) "
				<< "Data of size " << dataSize << " added to ICN buffer for "
				"CID " << icnId->print() << ". Stack size = "
				<< _itBuffer->second.size() << endl;
#endif
		_itBuffer->second.push(itemStruct);
	}

	_mutexBuffer.unlock();
}

void Namespace::_addPublishedScopePath(IcnId *icnId)
{
	_publishedScopePaths.insert((pair<string, bool>(icnId->str(), false)));
}

void Namespace::_publishScopePath(IcnId *icnId)
{
	string id, prefixId;
	size_t lengthIterator = 0;
	ostringstream ossId, ossPrefixId;
	ossPrefixId.str("");
	IcnId scopePath;

	while (lengthIterator < icnId->str().length())
	{
		// Get ID
		ossId.str("");

		for (size_t i = lengthIterator; i < (lengthIterator + ID_LEN); i++)
		{
			ossId << icnId->str()[i];
		}

		// Publish ID under father scope
		// check if information item has been reached
		if ((lengthIterator + ID_LEN) == icnId->str().length())
		{
			_blackadder->publish_info(hex_to_chararray(ossId.str()),
					hex_to_chararray(ossPrefixId.str()), DOMAIN_LOCAL, NULL, 0);
#ifdef BAMPERS_DEBUG
			scopePath = ossPrefixId.str().c_str();
			cout << "((BAMPERS)) "
					<< "Info item " << ossId.str() << " advertised under father"
					<< " scope " << scopePath.print() << endl;
#endif
		}
		else
		{
			_blackadder->publish_scope(hex_to_chararray(ossId.str()),
					hex_to_chararray(ossPrefixId.str()), DOMAIN_LOCAL, NULL, 0);
#ifdef BAMPERS_DEBUG
			scopePath = ossPrefixId.str().c_str();
			cout << "((BAMPERS)) "
					<< "Scope ID " << ossId.str() << " published under father "
					"scope " << scopePath.print() << endl;
#endif
		}

		// Prepare ID and PrefixID for next scope level
		ossPrefixId << ossId.str();
		lengthIterator += ID_LEN;
	}

	_addPublishedScopePath(icnId);
}
