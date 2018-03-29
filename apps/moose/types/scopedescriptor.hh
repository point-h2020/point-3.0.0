/*
 * scopedescriptor.hh
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

#ifndef APPS_MONITORING_SERVER_TYPES_SCOPEDESCRIPTOR_HH_
#define APPS_MONITORING_SERVER_TYPES_SCOPEDESCRIPTOR_HH_

#include <bampers/bampers.hh>
#include <string>

using namespace std;
/*!
 * \brief
 */
class ScopeDescriptor {
public:
	/*!
	 * \brief Constructor
	 */
	ScopeDescriptor();
	/*!
	 * \brief Deconstructor
	 */
	~ScopeDescriptor();
	/*!
	 * \brief Overload = operator
	 */
	void operator=(IcnId &scope);
	/*!
	 * \brief Obtain whether or not the ICN ID has been already published
	 *
	 * \return Boolean indicating the status
	 */
	bool published();
	/*!
	 * \brief Obtain whether or not MOOSE has already subscribed to this CID
	 *
	 * \return Boolean indicating the status
	 */
	bool subscribed();
	/*!
	 * \brief Set publication state of the ICN ID
	 */
	void setPublicationState(bool state);
	/*!
	 * \brief Set subscription state of the ICN ID
	 */
	void setSubscriptionState(bool state);
private:
	bool _published;/*!< Publication state to RV */
	bool _subscribed;/*!< Subscription state */
	IcnId _icnId; /*!< ICN ID hold in this class */
};

#endif /* APPS_MONITORING_SERVER_TYPES_SCOPEDESCRIPTOR_HH_ */
