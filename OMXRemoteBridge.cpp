/*
 * OMXRemoteBridge.cpp
 *
 *  Created on: Oct 2, 2013
 *      Author: rycus
 */

#include "OMXRemoteBridge.h"

COMXRemoteBridge::COMXRemoteBridge(
		OMXClock* clock,
		OMXReader* reader,
		OMXPlayerAudio* audio,
		OMXControl* ctrl,
		int port) : OMXThread() {

	m_listen_port = port;
	m_sockfd      = 0;

	m_exit_requested = 0;

	m_clock       = clock;
	m_reader      = reader;
	m_audio       = audio;
	m_control     = ctrl;
}

COMXRemoteBridge::~COMXRemoteBridge() {
	if(m_sockfd) close(m_sockfd);
}

void COMXRemoteBridge::AppendLong(char* buffer, long value) {
	int idx, shift;
	long mask, b;

	for (idx = 0; idx < 8; idx++) {
		shift = (8 - 1 - idx) * 8;
		mask  = 0xFF << shift;
		b     = (value & mask) >> shift;

		buffer[idx] = (unsigned char) (b & 0xFF);
	}
}

void COMXRemoteBridge::Handle(char* data, int len) {
	int  rsp_len = 0;
	char response[255];

	CLog::Log(LOGDEBUG,
			"COMXRemoteBridge::Handle - Remote Bridge handling incoming data: '%s' (%d)", data, len);

	// TODO implement this
	switch (data[0]) {
	case MSG_INIT:
	{
		long duration = m_reader->GetStreamLength();
		long volume   = m_audio->GetVolume(); // Volume in millibels
	    // double normalized_volume = pow(10, volume / 2000.0);
		printf("INIT:: %ld, %ld\n", volume, duration);

		response[rsp_len++] = MSG_INIT;

		AppendLong(response + rsp_len, duration);
		rsp_len += 8;

		AppendLong(response + rsp_len, volume);
		rsp_len += 8;

		break;
	}
	case MSG_EXIT:
		m_exit_requested = 1;
		m_control->pushLocalAction(KeyConfig::ACTION_EXIT);
		break;
	case MSG_PAUSE:
		m_control->pushLocalAction(KeyConfig::ACTION_PAUSE);
		break;
	case MSG_SPEED_DEC:
		m_control->pushLocalAction(KeyConfig::ACTION_DECREASE_SPEED);
		break;
	case MSG_SPEED_INC:
		m_control->pushLocalAction(KeyConfig::ACTION_INCREASE_SPEED);
		break;
	case MSG_SUB_DELAY_DEC:
		m_control->pushLocalAction(KeyConfig::ACTION_DECREASE_SUBTITLE_DELAY);
		break;
	case MSG_SUB_DELAY_INC:
		m_control->pushLocalAction(KeyConfig::ACTION_INCREASE_SUBTITLE_DELAY);
		break;
	case MSG_SUB_SHOW:
		m_control->pushLocalAction(KeyConfig::ACTION_TOGGLE_SUBTITLE);
		break;
	case MSG_VOLUME:
		// TODO
		break;
	case MSG_SEEK_TO:
		// TODO
		break;
	case MSG_POSITION:
	{
		long position = (unsigned) (m_clock->OMXMediaTime() * 1e-3);
		printf("POS: %ld\n", position);

		response[rsp_len++] = MSG_POSITION;
		AppendLong(response + rsp_len, position);
		rsp_len += 8;

		break;
	}
	}

	if (rsp_len) {
		int idx;
		char buflen[1] = { (unsigned char) (rsp_len & 0xFF) };

		write(m_sockfd, buflen, 1);
		write(m_sockfd, response, rsp_len);
		response[rsp_len] = 0;

		printf("WRITE: %s\n", response);
		printf("  HEX: ");
		for(idx=0; idx<rsp_len; idx++) {
			printf("%2X ", response[idx]);
		}
		printf("\n");
	}
}

void COMXRemoteBridge::Process() {
	bool connected = false;
	int  buf_size = 255;
	char buffer[buf_size];

	connected = Connect();

	if (connected) {
		while ( (!m_bStop) && (!m_exit_requested) ) {
			int rd = read(m_sockfd, buffer, buf_size);
			if (rd > 0) {
				buffer[rd] = 0;
				Handle(buffer, rd);
			} else if ( !m_bStop && (rd < 0) && (errno == EAGAIN) ) {
				continue; // Was timeout, retry
			} else {
				connected = false;
				break;
			}
		}

		CLog::Log(LOGDEBUG,
			"COMXRemoteBridge::Process - Remote Bridge exited receive loop");

		if (connected) {
			// Send exit message
			buffer[0] = MSG_EXIT;
			write(m_sockfd, buffer, 1);
		}
	}

}

bool COMXRemoteBridge::Connect() {
	struct sockaddr_in 	 serv_addr;
	struct hostent 		*server;

	CLog::Log(LOGDEBUG,
		"COMXRemoteBridge::Connect - Remote Bridge conntecting to port %d ...", m_listen_port);

	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockfd < 0) {
		CLog::Log(LOGWARNING,
				"COMXRemoteBridge::Connect - Failed to open socket [socket(..)]: %d", errno);
		return false;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(m_listen_port);

	server = gethostbyname("127.0.0.1");
	if (!server) {
		CLog::Log(LOGWARNING,
				"COMXRemoteBridge::Connect - Failed to open socket [gethostbyname(..)]: %d", errno);
		return false;
	}

	memcpy(&serv_addr.sin_addr, server->h_addr_list[0], server->h_length);

	struct timeval timeout;
	timeout.tv_sec  = 1; // 1 sec timeout
	timeout.tv_usec = 0;

	setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	if (connect(m_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		CLog::Log(LOGWARNING,
				"COMXRemoteBridge::Connect - Failed to open socket [connect(..)]: %d", errno);
		return false;
	}

	CLog::Log(LOGDEBUG,
		"COMXRemoteBridge::Connect - Remote Bridge connected on port %d", m_listen_port);

	return true;
}
