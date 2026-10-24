// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

void sanitizeString (
	char	*p)
{
	int	i;
	for (i = (int) strlen (p); i > 0 && isspace (p[i-1]); i--) p[i-1] = 0;
	while (*p) {
		if (!IsCharAlphaNumeric (*p) &&
		    *p != '.' && *p != '-' && *p != '_')
			*p = '_';
		p++;
	}
}

#define STAT0 "Below is a report on the work you have queued and any expected completion dates.\n"
#define STAT1 "The chance that one of the %d exponents you are testing will yield a %sprime is about 1 in %lld. "
#define STAT1a "The chance that the exponent you are testing will yield a %sprime is about 1 in %lld. "
#define STAT3 "No work queued up.\n"

void rangeStatusMessage (
	char	*buf,
	unsigned int buflen)		
{
	unsigned int tnum, ll_and_prp_cnt, lines_per_worker;
	int	mersennes;		
	double	prob, est;
	char	*orig_buf;

	if (! WORKERS_ACTIVE && !COMMUNICATION_THREAD) readIniFiles ();

	lines_per_worker = (unsigned int) IniGetInt (INI_FILE, "StatusLines", buflen / 62) / NUM_WORKERS;
	if (lines_per_worker < 3) lines_per_worker = 3;
	orig_buf = buf;
	ll_and_prp_cnt = 0;
	prob = 0.0;
	mersennes = TRUE;
	strcpy (buf, STAT0);
	buf += strlen (buf);

	for (tnum = 0; tnum < NUM_WORKERS; tnum++) {
	    struct work_unit *w;
	    unsigned int lines_output;
	    int truncated_status_msg;

	    lines_output = 0;
	    truncated_status_msg = FALSE;

	    if (NUM_WORKERS > 1) {
		sprintf (buf, "[Worker #%d]\n", tnum+1);
		buf += strlen (buf);
		lines_output++;
	    }

	    est = 0.0;
	    for (int pass = 0; pass <= 1; pass++)
	    for (w = NULL; ; ) {
		time_t	this_time;
		char	timebuf[80];
		unsigned int bits;

		w = getNextWorkToDoLine (tnum, w, SHORT_TERM_USE);
		if (w == NULL) break;
		if (w->work_type == WORK_NONE) continue;
		if (pass == 0 && w->work_type != WORK_CERT) continue;
		if (pass == 1 && w->work_type == WORK_CERT) continue;

		if (w->k != 1.0 || w->b != 2 || w->c != -1 || w->known_factors != NULL) mersennes = FALSE;

		bits = (unsigned int) w->sieve_depth;
		if (bits < 32) bits = 32;
		if (w->work_type == WORK_TEST) {
			ll_and_prp_cnt++;
			prob += (bits - 1) * 1.733 * (w->pminus1ed ? 1.04 : 1.0) / (log2(w->k) + log2(w->b) * w->n);
		}
		if (w->work_type == WORK_DBLCHK) {
			ll_and_prp_cnt++;
			prob += (bits - 1) * 1.733 * ERROR_RATE * (w->pminus1ed ? 1.04 : 1.0) / (log2(w->k) + log2(w->b) * w->n);
		}
		if (w->work_type == WORK_PRP) {
			ll_and_prp_cnt++;
			if (!w->prp_dblchk)
				prob += (bits - 1) * 1.733 * (w->pminus1ed ? 1.04 : 1.0) / (log2(w->k) + log2(w->b) * w->n);
			else
				prob += (bits - 1) * 1.733 * PRP_ERROR_RATE * (w->pminus1ed ? 1.04 : 1.0) / (log2(w->k) + log2(w->b) * w->n);
		}

		est += work_estimate (tnum, w);

		if ((unsigned int) (buf - orig_buf) >= buflen - 200 || lines_output >= lines_per_worker-1) {
			if (! truncated_status_msg) {
				strcpy (buf, "More...\n");
				buf += strlen (buf);
				truncated_status_msg = TRUE;
			}
			continue;
		}

		if (w->n) {
			gw_as_string (buf, w->k, w->b, w->n, w->c);
			buf += strlen (buf);
			if (w->work_type == WORK_PRP && w->known_factors) {
				strcpy (buf, "/known_factors");
				buf += strlen (buf);
			}
			strcpy (buf, ", ");
			buf += strlen (buf);
		}

		if (w->work_type == WORK_ECM) {
			if (w->gmp_ecm_file) sprintf (buf, "ECM stage 2 on file %s", w->gmp_ecm_file);
			else sprintf (buf, "ECM %d curve%s B1=%" PRIu64, w->curves_to_do, w->curves_to_do == 1 ? "" : "s",
				      w->B1 * IniGetInt (INI_FILE, "ECMBoundsMultiplier", 1));
		}
		else if (w->work_type == WORK_PMINUS1)
			sprintf (buf, "P-1 B1=%" PRIu64, w->B1);
		else if (w->work_type == WORK_PPLUS1)
			sprintf (buf, "P+1 B1=%" PRIu64, w->B1);
		else if (w->work_type == WORK_FACTOR)
			sprintf (buf, "factor from 2^%d to 2^%d", (int) w->sieve_depth, (int) w->factor_to);
		else
			strcpy (buf, w->work_type == WORK_PFACTOR ? "P-1" :
				     w->work_type == WORK_TEST ||
				     w->work_type == WORK_ADVANCEDTEST ? "Lucas-Lehmer test" :
				     w->work_type == WORK_DBLCHK ? "Double-check" :
				     w->work_type == WORK_CERT ? "Certify" :
				     w->prp_dblchk ? "PRPDC" : "PRP");
		buf += strlen (buf);

		time (&this_time);
		this_time += (time_t) est;
		if (this_time < 2147483640.0) {
			strcpy (timebuf, ctime (&this_time));
			safe_strcpy (timebuf+16, timebuf+19);
		} else
			strcpy (timebuf, "after Jan 19 2038\n");
		sprintf (buf, ", %s", timebuf);
		buf += strlen (buf);
		lines_output++;
	    }

	    if (est == 0.0 && ! truncated_status_msg) {
		strcpy (buf, STAT3);
		buf += strlen (buf);
	    }
	}

	if (ll_and_prp_cnt == 1)
		sprintf (buf+strlen(buf), STAT1a, mersennes ? "Mersenne " : "", (long long) (1.0 / prob));
	if (ll_and_prp_cnt > 1)
		sprintf (buf+strlen(buf), STAT1, ll_and_prp_cnt, mersennes ? "Mersenne " : "", (long long) (1.0 / prob));
}

int min_cores_for_work_pref (
	int	work_pref)
{
	int	cores;

	cores = 1;

	if (work_pref == PRIMENET_WP_LL_100M || work_pref == PRIMENET_WP_PRP_100M) {
		if (HW_NUM_CORES < 8) cores = HW_NUM_CORES;
		else cores = 4;
	}

	cores = cores * 24 / CPU_HOURS;
	if (cores > (int) HW_NUM_CORES) cores = HW_NUM_CORES;

	return (cores);
}
