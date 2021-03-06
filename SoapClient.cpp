/*
 * libnet++
 *
 * Copyright (C) 2007-2013 Joachim Wilke <libnet@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "SoapClient.h"

namespace network {

SoapClient::SoapClient(const std::string &host, int port)
: HttpClient{host, port} {
}

SoapClient::~SoapClient() {
}

std::string SoapClient::post(const std::string &request, const std::string &action, const std::string &body) {
	header_t header = {
			{ "Content-Type", "text/xml; charset=utf-8" },
			{ "SOAPAction", action },
	};
	return sendRequest(request, std::stringstream().flush() << body, header);
}

}
