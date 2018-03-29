/*
 * scopes.hh
 *
 *  Created on: 12 Feb 2016
 *      Author: Sebastian Robitzsch <sebastian.robitzsch@interdigital.com>
 *
 * This file is part of the ICN application MOnitOring SErver (MOOSE) which
 * comes with Blackadder.
 *
 * MOOSE is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * MOOSE is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * MOOSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APPS_MONITORING_SERVER_SCOPES_HH_
#define APPS_MONITORING_SERVER_SCOPES_HH_

#include <boost/thread/mutex.hpp>
#include <log4cxx/logger.h>
#include <unordered_map>

#include "scopedescriptor.hh"

using namespace std;
/*!
 * \brief Stateful class to keep track about scopes, scope paths and their
 * publication state
 *
 * Terminology:
 * Scope:
 * TODO write scope description
 * Scope Path:
 * TODO write scope path description
 * ICN ID:
 * TODO write ICN ID
 *
 */
class Scopes {
public:
	/*!
	 * \brief Constructor
	 */
	Scopes(log4cxx::LoggerPtr logger);
	/*!
	 * \brief Destructor
	 */
	~Scopes();
	/*!
	 * \brief Add an ICN ID to the list of scopes
	 */
	void add(IcnId &icnId);
	/*!
	 * \brief Add an ICN ID to the list of scopes
	 *
	 * This is a wrapper method which calls Scopes::add(IcnId &icnId)
	 */
	void add(string icnIdString);
	/*!
	 * \brief Erase an ICN ID from the list of scopes
	 */
	void erase(IcnId &icnId);
	/*!
	 * \brief Erase an ICN ID from the list of scopes
	 *
	 * This is a wrapper method which calls Scopes::erase(IcnId &icnId)
	 */
	void erase(string icnIdString);
	/*!
	 * \brief Check if ICN ID has been already published to RV
	 *
	 * \param icnId The ICN ID which should be checked
	 *
	 * \return Boolean indicating whether or not the ICN ID has been already
	 * published to RV
	 */
	bool published(IcnId &icnId);
	/*!
	 * \brief Check if MOOSE has been already subscribed to a particular ICN ID
	 *
	 * Note, there is no difference to this method whether the given ICN ID has
	 * a scope as the last ID or an information item
	 *
	 * \param icnId The ICN ID which is to be checked
	 *
	 * \return Boolean indicating whether or not MOOSE has already subscribed to
	 * the given ICN ID
	 */
	bool subscribed(IcnId &icId);
	/*!
	 * \brief set publication state for a particular scope path
	 *
	 * \param icnId The ICN ID for which the state should be set
	 * \param state The publication state to be set
	 */
	void setPublicationState(IcnId &icnId, bool state);
	/*!
	 * \brief Set subscription state of the ICN ID
	 *
	 * \param icnId The ICN ID for which the subscription state should be set
	 * \param state The subscription state to be set
	 */
	void setSubscriptionState(IcnId &icnId, bool state);
private:
	log4cxx::LoggerPtr _logger;
	unordered_map<uint32_t, ScopeDescriptor> _scopesMap;
	unordered_map<uint32_t, ScopeDescriptor>::iterator _itScopesMap;
	boost::mutex _mutex;/*!< Allow transaction sage R/W operations */
};

#endif /* APPS_MONITORING_SERVER_SCOPES_HH_ */
