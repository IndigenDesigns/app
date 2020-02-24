#include <re.h>
#include <baresip.h>
#include <string.h>
#include <math.h>

#include "webapp.h"


#define MAX_METERS 32

static float bias = 1.0f;
static float peaks[MAX_METERS] = {0};
static float sent_peaks[MAX_METERS] = {0};

static struct tmr tmr;


/* Read and reset the recent peak sample */
static void webapp_read_peaks(void)
{
	memcpy(sent_peaks, peaks, sizeof(peaks));
	memset(peaks, 0, sizeof(peaks));
}


void webapp_ws_meter(const struct websock_hdr *hdr,
				     struct mbuf *mb, void *arg)
{
	struct odict *cmd = NULL;
	int err = 0;

	err = json_decode_odict(&cmd, DICT_BSIZE, (const char *)mbuf_buf(mb),
			mbuf_get_left(mb), MAX_LEVELS);
	if (err)
		goto out;

out:
	mem_deref(cmd);
}

int64_t slrtaudio_record_get_timer(void);
static void write_ws(void)
{
	int64_t record_time;
	int n;
	int i;
	float db;
	char one_peak[100];
	char p[2048];
	int hours;
	int min;
	int sec;
	int msec;

	/* Record time */
	record_time = slrtaudio_record_get_timer();

	hours = (int)(record_time / 1000 / 3600);
	min = (int)(record_time / 1000 / 60) - (hours * 60); 
	sec = (int)(record_time / 1000) - (hours * 3600) - (min * 60);
	msec = (int)(record_time) - (hours * 3600 * 1000) - (min * 60 * 1000) - (sec * 1000);

//	warning("record time: %d:%02d:%02d:%03d\n", hours,min,sec,msec);

	p[0] = '\0';
	re_snprintf(one_peak, 100, "%d:%02d:%02d:%03d 0 ", hours, min, sec, msec);
	strcat((char*)p, one_peak);

	for (i=0; i<MAX_METERS; i++) {
		db = 20.0f * log10f(sent_peaks[i] * bias);
		re_snprintf(one_peak, 100, "%f ", db);
		strcat((char*)p, one_peak);
	}
	n = strlen(p);
	p[n-1] = '\0'; /* remove trailing space */

	ws_send_all(WS_METER, p);
}


static void tmr_handler(void *arg)
{
	tmr_start(&tmr, 150, tmr_handler, NULL);
	webapp_read_peaks();
	write_ws();
}


void ws_meter_process(unsigned int ch, float *in, unsigned long nframes)
{
	unsigned int i;

	for (i = 0; i < nframes; i=i+2) {
		const float s = fabs(in[i]);
		if (s > peaks[ch]) {
			peaks[ch] = s;
		}
	}
}


void webapp_ws_meter_init(void)
{
	tmr_init(&tmr);
	tmr_start(&tmr, 150, tmr_handler, NULL);
}


void webapp_ws_meter_close(void)
{
	tmr_cancel(&tmr);
}
