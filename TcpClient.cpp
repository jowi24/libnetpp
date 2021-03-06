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

#define BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM
#include "TcpClient.h"

#include <liblog++/Log.h>

namespace network {

void TcpClient::connectStream() {
	DBG("Connecting to " << host << ":" << port);
	std::string sPort = static_cast<std::stringstream&>(std::stringstream().flush() << port).str();
	stream = new boost::asio::ip::tcp::iostream(host, sPort);
	if (!(*stream))
		throw std::runtime_error(stream->error().message());
	connected = true;
}

TcpClient::TcpClient(const std::string &host, int port)
: host{host}, port{port}, connected{false}, stream{nullptr} {
}


TcpClient::~TcpClient() {
	disconnectStream();
}

std::string TcpClient::readLine(bool removeNewline) {
	if (!connected)
		connectStream();
	std::string line;
	std::getline(*stream, line);
	if (line.length() > 0 && removeNewline)
		line.erase(line.end()-1, line.end());
	return line;
}

void TcpClient::expireStreamNow() {
	if (stream)
		stream->expires_from_now(boost::posix_time::seconds(0));
}

void TcpClient::disconnectStream() {
	if (stream && connected) {
		DBG("Disconnecting from " << host << ":" << port);
		stream->close();
		delete stream;
	}
	connected = false;
}

void TcpClient::write(const std::string &data) {
	if (!connected)
		connectStream();
	*stream << data;
}

}
