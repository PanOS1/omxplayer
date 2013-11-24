/*
 * OMXRemoteBridge.h
 *
 *  Created on: Oct 2, 2013
 *      Author: rycus
 */

#ifndef OMXREMOTEBRIDGE_H_
#define OMXREMOTEBRIDGE_H_

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "utils/log.h"
#include "utils/Strprintf.h"

#include "OMXThread.h"
#include "OMXControl.h"
#include "OMXReader.h"
#include "OMXClock.h"
#include "OMXAudio.h"
#include "OMXPlayerSubtitles.h"
#include "KeyConfig.h"

/* Message identifiers */
#define MSG_EXIT          0xFE
#define MSG_INIT          0x01
#define MSG_PAUSE         0x11
#define MSG_SPEED_DEC     0x12
#define MSG_SPEED_INC     0x13
#define MSG_SUB_DELAY_DEC 0x14
#define MSG_SUB_DELAY_INC 0x15
#define MSG_SUB_SHOW      0x16
#define MSG_VOLUME        0x21
#define MSG_SEEK_TO       0x22
#define MSG_PLAYER_STATE  0x31

/* Remote Bridge thread */

class COMXRemoteBridge : public OMXThread {
public:
	COMXRemoteBridge(
			OMXClock* clock,
			OMXReader* reader,
			OMXPlayerAudio* audio,
			OMXControl* ctrl,
			OMXPlayerSubtitles* subtitles,
			int port,
			bool osd);
	virtual ~COMXRemoteBridge();

	int GetSeekPosition();
private:
	int m_listen_port;
	int m_sockfd;
	int m_exit_requested;
	int seekPosition = 0;

	OMXClock*       m_clock;
	OMXReader*      m_reader;
	OMXPlayerAudio* m_audio;
	OMXControl*     m_control;
	OMXPlayerSubtitles* m_subtitles;

	bool m_osd;

	bool Connect();
	void Process();
	void Handle(char* data, int len);
	void AppendLong(char* buffer, long value);
	long ReadLong(char* buffer, int offset);
};

#endif /* OMXREMOTEBRIDGE_H_ */
