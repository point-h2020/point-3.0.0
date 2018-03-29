/*
 * elementtype.cc
 *
 *  Created on: 26 Apr 2017
 *      Author: user
 */

#include "elementtype.hh"

ElementType::ElementType(IcnId cId)
	: _cId(cId)
{
	_nodeRole = (node_role_t)atoi(_cId.str().substr(2 * 16, 16).c_str());
	_elementType = _convert(_nodeRole);
}

ElementType::~ElementType() {}

element_type_t ElementType::elementType()
{
	return _elementType;
}

element_type_t ElementType::_convert(node_role_t nodeRole)
{
	element_type_t elementType;

	switch (nodeRole)
	{
	case NODE_ROLE_GW:
		elementType = ELEMENT_TYPE_GW;
		break;
	case NODE_ROLE_FN:
		elementType = ELEMENT_TYPE_FN;
		break;
	case NODE_ROLE_NAP:
		elementType = ELEMENT_TYPE_NAP;
		break;
	case NODE_ROLE_RV:
		elementType = ELEMENT_TYPE_RV;
		break;
	case NODE_ROLE_SERVER:
		elementType = ELEMENT_TYPE_SERVER;
		break;
	case NODE_ROLE_TM:
		elementType = ELEMENT_TYPE_TM;
		break;
	case NODE_ROLE_UE:
		elementType = ELEMENT_TYPE_UE;
		break;
	default:
		elementType = ELEMENT_TYPE_UNKNOWN;
	}

	return elementType;
}
