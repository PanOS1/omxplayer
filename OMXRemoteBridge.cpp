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
		OMXPlayerSubtitles* subtitles,
		int port,
		bool osd) : OMXThread() {

	m_listen_port = port;
	m_sockfd      = 0;

	m_exit_requested = 0;

	m_clock       = clock;
	m_reader      = reader;
	m_audio       = audio;
	m_control     = ctrl;
	m_subtitles   = subtitles;

	seekPosition = -1;
	m_osd = osd;
}

COMXRemoteBridge::~COMXRemoteBridge() {
	if(m_sockfd) close(m_sockfd);
}

void COMXRemoteBridge::AppendLong(char* buffer, long value) {
	int idx, shift;
	long mask, b;
	bool negative;

	negative  = value < 0;
	if (negative) {
		value = -value;
	}

	for (idx = 0; idx < 4; idx++) {
		shift = (4 - 1 - idx) * 8;
		mask  = 0xFF << shift;
		b     = (value & mask) >> shift;

		buffer[idx] = (unsigned char) (b & 0xFF);
	}

	if(negative) {
		buffer[0] |= 0x80;
	}
}

long COMXRemoteBridge::ReadLong(char* buffer, int offset) {
	long val;
	int idx, shift;
	bool negative;

	negative = buffer[offset] & 0x80;
	buffer[offset] &= 0x7F;

	val = 0;

	for (idx = 0; idx < 4; idx++) {
		shift = (4 - 1 - idx) * 8;
		val = val + ((buffer[offset + idx]) << shift);
	}

	return negative ? -val : val;
}

void COMXRemoteBridge::Handle(char* data, int len) {
	int  rsp_len = 0;
	char response[255];

	CLog::Log(LOGDEBUG,
			"COMXRemoteBridge::Handle - Remote Bridge handling incoming data: '%s' (%d)", data, len);

	switch (data[0]) {
	case MSG_INIT:
	{
		long duration = m_reader->GetStreamLength();
		long volume 	= static_cast<long>(2000.0 * log10(m_audio->GetVolume()));

		response[rsp_len++] = MSG_INIT;

		AppendLong(response + rsp_len, duration);
		rsp_len += 4;

		AppendLong(response + rsp_len, volume);
		rsp_len += 4;

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
	{
		long volume = ReadLong(data, 1);

		double dv = pow(10, volume / 2000.0);
		m_audio->SetVolume(dv);

		if(m_osd) {
			m_subtitles->DisplayText(strprintf("Volume: %.2f dB", volume / 100.0f), 1000);
		}

		break;
	}
	case MSG_SEEK_TO:
	{
		long position = ReadLong(data, 1);

		Lock();
		seekPosition = position;
		UnLock();

		break;
	}
	case MSG_PLAYER_STATE:
	{
		long position 	= (unsigned) (m_clock->OMXMediaTime() * 1e-3);
		long volume 	= static_cast<long>(2000.0 * log10(m_audio->GetVolume()));

		response[rsp_len++] = MSG_PLAYER_STATE;

		AppendLong(response + rsp_len, position);
		rsp_len += 4;

		AppendLong(response + rsp_len, volume);
		rsp_len += 4;

		if( m_clock->OMXIsPaused() ) {
			response[rsp_len] = 'P';
		} else {
			response[rsp_len] = 'R';
		}
		rsp_len++;

		break;
	}
	}

	if (rsp_len) {
		int idx;
		char buflen[1] = { (unsigned char) (rsp_len & 0xFF) };

		write(m_sockfd, buflen, 1);
		write(m_sockfd, response, rsp_len);
		response[rsp_len] = 0;
	}
}

int COMXRemoteBridge::GetSeekPosition() {
	int result = 0;
	Lock();
	result = seekPosition;
	seekPosition = -1;
	UnLock();
	return result;
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

		// Send exit message
		buffer[0] = MSG_EXIT;
		write(m_sockfd, buffer, 1);

		close(m_sockfd);
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
