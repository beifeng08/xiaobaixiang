// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

struct cert_state {
	int	thread_num;		
	gwhandle gwdata;
	gwnum	x;			
	unsigned long counter;		
	unsigned long units_bit;	
	unsigned long error_count;	
};

#define CERT_MAGICNUM		0x8f729ab1
#define CERT_VERSION		1

int writeCertSaveFile (			
	struct cert_state *cs,		
	writeSaveFileState *write_save_file_state,
	struct work_unit *w)		
{
	int	fd;
	uint32_t sum = 0;
	char	buf[512];

	fd = openWriteSaveFile (write_save_file_state);
	if (fd < 0) {
		sprintf (buf, "Unable to create cert save file: %s\n", write_save_file_state->base_filename);
		OutputBoth (cs->thread_num, buf);
		OutputBothErrno (cs->thread_num);
		return (FALSE);
	}

	if (!write_header (fd, CERT_MAGICNUM, CERT_VERSION, w)) goto writeerr;
	if (!write_long (fd, cs->error_count, &sum)) goto writeerr;
	if (!write_long (fd, cs->counter, &sum)) goto writeerr;
	if (!write_long (fd, cs->units_bit, &sum)) goto writeerr;
	if (!write_gwnum (fd, &cs->gwdata, cs->x, &sum)) {
		sprintf (buf, "Error writing FFT data named x to cert save file %s\n", write_save_file_state->base_filename);
		OutputBoth (cs->thread_num, buf);
		OutputBothErrno (cs->thread_num);
		goto err;
	}

	if (!write_checksum (fd, sum)) goto writeerr;

	closeWriteSaveFile (write_save_file_state, fd);
	return (TRUE);

writeerr:
	sprintf (buf, WRITEFILEERR, write_save_file_state->base_filename);
	OutputBoth (cs->thread_num, buf);
	OutputBothErrno (cs->thread_num);
err:	deleteWriteSaveFile (write_save_file_state, fd);
	return (FALSE);
}

int readCertSaveFile (			
	struct cert_state *cs,		
	char	*filename,		
	struct work_unit *w)		
{
	int	fd;
	uint32_t version, sum = 0, filesum;

	fd = _open (filename, _O_BINARY | _O_RDONLY);
	if (fd <= 0) return (FALSE);

	if (!read_magicnum (fd, CERT_MAGICNUM)) goto err;
	if (!read_header (fd, &version, w, &filesum)) goto err;
	if (version == 0 || version > CERT_VERSION) goto err;

	if (!read_long (fd, &cs->error_count, &sum)) goto err;
	if (!read_long (fd, &cs->counter, &sum)) goto err;
	if (!read_long (fd, &cs->units_bit, &sum)) goto err;
	if (!read_gwnum (fd, &cs->gwdata, cs->x, &sum)) goto err;

	if (filesum != sum) goto err;
	_close (fd);
	return (TRUE);
err:	_close (fd);
	return (FALSE);
}

int cert (
	int	thread_num,		
	struct PriorityInfo *sp_info,	
	struct work_unit *w,		
	int	pass)			
{
	struct cert_state cs;
	int	first_iter_msg, res, stop_reason;
	int	echk, near_fft_limit;
	unsigned long iters;
	double	timers[2];
	double	inverse_explen;
	double	reallyminerr = 1.0;
	double	reallymaxerr = 0.0;
	readSaveFileState read_save_file_state; 
	writeSaveFileState write_save_file_state; 
	char	filename[32];
	char	buf[1000], JSONbuf[4000], fft_desc[200];
	double	allowable_maxerr, output_frequency, output_title_frequency;
	char	string_rep[80];
	int	error_count_messages;
	hash256_t hash;

	memset (&cs, 0, sizeof (cs));
	cs.thread_num = thread_num;
	gwinit (&cs.gwdata);

	if (w->k != 1.0 || w->b != 2 || w->c != -1) {
		
		OutputStr (thread_num, "Certification of non-Mersenne numbers not supported\n");
		goto abandon_work;
	}

	tempFileName (w, filename);
	writeSaveFileStateInit (&write_save_file_state, filename, NUM_JACOBI_BACKUP_FILES);

	if (IniGetInt (INI_FILE, "UseLargePages", 0)) gwset_use_large_pages (&cs.gwdata);
	if (HYPERTHREAD_LL) sp_info->normal_work_hyperthreading = TRUE, gwset_will_hyperthread (&cs.gwdata, 2);
	gwset_bench_cores (&cs.gwdata, HW_NUM_CORES);
	gwset_bench_workers (&cs.gwdata, NUM_WORKERS);
	if (ERRCHK) gwset_will_error_check (&cs.gwdata);
	else gwset_will_error_check_near_limit (&cs.gwdata);
	gwset_num_threads (&cs.gwdata, get_worker_num_threads (thread_num, HYPERTHREAD_LL));
	gwset_thread_callback (&cs.gwdata, SetAuxThreadPriority);
	gwset_thread_callback_data (&cs.gwdata, sp_info);
	gwset_minimum_fftlen (&cs.gwdata, w->minimum_fftlen);
	gwset_safety_margin (&cs.gwdata, IniGetFloat (INI_FILE, "CertificationSafetyMargin", 0.0));
	gwset_use_spin_wait (&cs.gwdata, IniGetInt (INI_FILE, "SpinWait", 0));
	res = gwsetup (&cs.gwdata, w->k, w->b, w->n, w->c);

	if (res) {
		char	string_rep[80];
		gw_as_string (string_rep, w->k, w->b, w->n, w->c);
		sprintf (buf, "Certification cannot initialize FFT code for %s, errcode=%d\n", string_rep, res);
		OutputBoth (thread_num, buf);
		gwerror_text (&cs.gwdata, res, buf, sizeof (buf) - 1);
		strcat (buf, "\n");
		OutputBoth (thread_num, buf);
		goto abandon_work;
	}

	set_memory_usage (thread_num, 0, cvt_gwnums_to_mem (&cs.gwdata, 1));

	cs.x = gwalloc (&cs.gwdata);
	if (cs.x == NULL) {
		OutputStr (thread_num, "Error allocating memory for FFT data.\n");
		stop_reason = STOP_OUT_OF_MEM;
		goto exit;
	}

	strcpy (string_rep, gwmodulo_as_string (&cs.gwdata));

	sprintf (buf, "Cert %s", string_rep);
	title (thread_num, buf);

	readSaveFileStateInit (&read_save_file_state, thread_num, filename);
	for ( ; ; ) {

		if (! saveFileExists (&read_save_file_state)) {
			
			cs.counter = 0;
			cs.error_count = 0;
			first_iter_msg = FALSE;
			break;
		}

		if (readCertSaveFile (&cs, read_save_file_state.current_filename, w)) {
			first_iter_msg = TRUE;
			break;
		}

		else
			saveFileBad (&read_save_file_state);
	}

	gwfft_description (&cs.gwdata, fft_desc);
	strcpy (buf, (cs.counter == 0) ? "Starting " : "Resuming ");
	sprintf (buf+strlen(buf), "certification of %s using %s\n", string_rep, fft_desc);
	OutputStr (thread_num, buf);

	clear_timers (timers, sizeof (timers) / sizeof (timers[0]));

	strcpy (w->stage, "CERT");
	inverse_explen = 1.0 / (double) w->cert_squarings;
	w->pct_complete = (double) cs.counter * inverse_explen;
	calc_output_frequencies (&cs.gwdata, &output_frequency, &output_title_frequency);

	near_fft_limit = exponent_near_fft_limit (&cs.gwdata);

	allowable_maxerr = IniGetFloat (INI_FILE, "MaxRoundoffError", (float) (near_fft_limit ? 0.421875 : 0.40625));

	error_count_messages = IniGetInt (INI_FILE, "ErrorCountMessages", 3);

	if (cs.counter == 0) {
		int	i, residue_size;
		uint32_t *array;		
		uint32_t arraylen;		
		char	md5[33], residue_md5[33];

		residue_size = divide_rounding_up ((int) ceil(cs.gwdata.bit_length), 8);
		arraylen = divide_rounding_up (residue_size, 4);
		array = (uint32_t *) malloc (arraylen * sizeof(uint32_t));
		array[arraylen-1] = 0;		

		if (ProofGetData (w->assignment_uid, array, residue_size, md5) != PRIMENET_NO_ERROR || !isHex (md5)) {
			OutputBoth (thread_num, "Error getting CERT starting value.\n");
			free (array);
			goto retry_work;
		}
		md5_hexdigest_buffer (residue_md5, array, residue_size);
		strupper (md5);
		strupper (residue_md5);
		if (strcmp (md5, residue_md5) != 0) {
			OutputBoth (thread_num, "MD5 of downloaded starting value does not match.\n");
			free (array);
			goto retry_work;
		}

		for (i = 0; i < (int) arraylen; i++) {
			uint32_t val;
			val = ((unsigned char *)&array[i])[3];
			val = (val << 8) + ((unsigned char *)&array[i])[2];
			val = (val << 8) + ((unsigned char *)&array[i])[1];
			val = (val << 8) + ((unsigned char *)&array[i])[0];
			array[i] = val;
		}

		binarytogw (&cs.gwdata, array, arraylen, cs.x);
		free (array);

		srand ((unsigned) time (NULL));
		cs.units_bit = ((rand () << 16) + rand ()) % w->n;
		gwrotate_left (&cs.gwdata, cs.x, cs.units_bit);
	}

	iters = 0;
	while (cs.counter < (unsigned long) w->cert_squarings) {
		int	saving, actual_frequency;

		stop_reason = stopCheck (thread_num);
		saving = stop_reason || testSaveFilesFlag (thread_num);

		echk = ERRCHK || saving || near_fft_limit || (cs.counter & 127) == 0;
		gw_clear_maxerr (&cs.gwdata);

		timers[1] = 0.0;
		start_timer (timers, 1);

		gwstartnextfft (&cs.gwdata, !saving && cs.counter != w->cert_squarings-1);
		gwsetnormroutine (&cs.gwdata, 0, echk, 0);
		gwsquare (&cs.gwdata, cs.x);
		cs.units_bit <<= 1;
		if (cs.units_bit >= w->n) cs.units_bit -= w->n;

		end_timer (timers, 1);
		timers[0] += timers[1];
		iters++;

		if (echk) {
			if (gw_get_maxerr (&cs.gwdata) < reallyminerr) reallyminerr = gw_get_maxerr (&cs.gwdata);
			if (gw_get_maxerr (&cs.gwdata) > reallymaxerr) reallymaxerr = gw_get_maxerr (&cs.gwdata);
		}

		if (echk && gw_get_maxerr (&cs.gwdata) > allowable_maxerr) {
			char	msg[100];
			sprintf (msg, ERRMSG1C, gw_get_maxerr (&cs.gwdata), allowable_maxerr);
			sprintf (buf, ERRMSG0, cs.counter+1, (long) w->cert_squarings, msg);
			OutputBoth (thread_num, buf);
			inc_error_count (1, &cs.error_count);
		}

		cs.counter++;
		w->pct_complete = (double) cs.counter * inverse_explen;

		actual_frequency = (int) (ITER_OUTPUT * output_title_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if (cs.counter % actual_frequency == 0 || first_iter_msg) {
			sprintf (buf, "%.*f%% of cert %s", (int) PRECISION, trunc_percent (w->pct_complete), string_rep);
			title (thread_num, buf);
		}

		actual_frequency = (int) (ITER_OUTPUT * output_frequency);
		if (actual_frequency < 1) actual_frequency = 1;
		if (cs.counter % actual_frequency == 0 || first_iter_msg) {
			sprintf (buf, "Iteration: %ld / %ld [%.*f%%]",
				 cs.counter, (long) w->cert_squarings, (int) PRECISION, trunc_percent (w->pct_complete));
			
			if ((error_count_messages & 0xFF) == 1)
				make_error_count_message (cs.error_count, error_count_messages, buf + strlen (buf),
							  (int) (sizeof (buf) - strlen (buf)));
			
			if (first_iter_msg) {
				strcat (buf, ".\n");
				clear_timer (timers, 0);
				first_iter_msg = FALSE;
			}
			
			else if (!CLASSIC_OUTPUT) {
				double speed;
				
				if ((OUTPUT_ROUNDOFF || ERRCHK) && reallymaxerr >= 0.001) {
					sprintf (buf+strlen(buf), ", roundoff: %5.3f", reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				
				speed = timer_value (timers, 0) / (double) iters;
				sprintf (buf+strlen(buf), ", ms/iter: %6.3f", speed * 1000.0);
				clear_timer (timers, 0);
				iters = 0;
				
				formatETA ((w->cert_squarings - cs.counter) * speed, buf+strlen(buf));
				strcat (buf, "\n");
			}
			
			else {
				
				if (ERRCHK && cs.counter > 30) {
					sprintf (buf+strlen(buf), ".  Round off: %10.10f to %10.10f", reallyminerr, reallymaxerr);
					if (!CUMULATIVE_ROUNDOFF) reallyminerr = 1.0, reallymaxerr = 0.0;
				}
				if (CUMULATIVE_TIMING) {
					strcat (buf, ".  Total time: ");
					print_timer (timers, 0, buf, TIMER_NL);
				} else {
					strcat (buf, ".  Per iteration time: ");
					divide_timer (timers, 0, iters);
					print_timer (timers, 0, buf, TIMER_NL | TIMER_CLR);
					iters = 0;
				}
			}
			OutputStr (thread_num, buf);

			if ((error_count_messages & 0xFF) >= 2 &&
			    make_error_count_message (cs.error_count, error_count_messages, buf, sizeof (buf)))
				OutputStr (thread_num, buf);
		}

		if (saving) {
			writeCertSaveFile (&cs, &write_save_file_state, w);
		}

		if (stop_reason) {
			sprintf (buf, "Stopping certification of %s at iteration %ld [%.*f%%]\n",
				 string_rep, cs.counter, (int) PRECISION, trunc_percent (w->pct_complete));
			OutputStr (thread_num, buf);
			goto exit;
		}
	}

	if (!gwrotate_right (&cs.gwdata, cs.x, cs.units_bit) || !sha3_gwnum (&cs.gwdata, cs.x, &hash)) {
		OutputBoth (thread_num, ERRMSG8);
		inc_error_count (2, &cs.error_count);
		goto retry_work;
	}

	gwfree (&cs.gwdata, cs.x);

	sprintf (buf, "%s certification hash value %s.", string_rep, hash_to_string (hash));
	sprintf (buf+strlen(buf), " Wh%d: %08lX,", PORT, SEC1 (w->n));
	if (cs.units_bit) sprintf (buf+strlen(buf), "%ld,", cs.units_bit);
	sprintf (buf+strlen(buf), "%08lX\n", cs.error_count);
	OutputStr (thread_num, buf);
	formatMsgForResultsFile (buf, w);

	if (IniGetInt (INI_FILE, "OutputCerts", 0))
		writeResults (buf);

	sprintf (JSONbuf, "{");
	sprintf (JSONbuf+strlen(JSONbuf), "\"worktype\":\"Cert\"");
	JSONaddExponent (JSONbuf, w);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"sha3-hash\":\"%s\"", hash_to_string (hash));
	sprintf (JSONbuf+strlen(JSONbuf), ", \"fft-length\":%lu", cs.gwdata.FFTLEN);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"squarings\":%d", w->cert_squarings);
	if (cs.units_bit) sprintf (JSONbuf+strlen(JSONbuf), ", \"shift-count\":%ld", cs.units_bit);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"error-code\":\"%08lX\"", cs.error_count);
	sprintf (JSONbuf+strlen(JSONbuf), ", \"security-code\":\"%08lX\"", SEC1(w->n));
	JSONaddProgramTimestamp (JSONbuf);
	JSONaddExponentKnownFactors (JSONbuf, w);
	JSONaddUserComputerAID (JSONbuf, w);
	strcat (JSONbuf, "}");
	if (IniGetInt (INI_FILE, "OutputJSONCerts", 1)) writeResultsJSON (JSONbuf);

	{
		struct primenetAssignmentResult pkt;
		memset (&pkt, 0, sizeof (pkt));
		strcpy (pkt.computer_guid, COMPUTER_GUID);
		strcpy (pkt.assignment_uid, w->assignment_uid);
		strcpy (pkt.message, buf);
		pkt.result_type = PRIMENET_AR_CERT;
		pkt.k = w->k;
		pkt.b = w->b;
		pkt.n = w->n;
		pkt.c = w->c;
		strcpy (pkt.cert_hash, hash_to_string (hash));
		sprintf (pkt.error_count, "%08lX", cs.error_count);
		pkt.shift_count = cs.units_bit;
		pkt.fftlen = gwfftlen (&cs.gwdata);
		pkt.done = TRUE;
		strcpy (pkt.JSONmessage, JSONbuf);
		spoolMessage (PRIMENET_ASSIGNMENT_RESULT, &pkt);
	}

	unlinkSaveFiles (&write_save_file_state);
	goto work_complete;

retry_work:
	echk = IniSectionGetInt (INI_FILE, SEC_Internals, KEY_CertErrorCount, 0) + 1;
	IniSectionWriteInt (INI_FILE, SEC_Internals, KEY_CertErrorCount, echk);
	if (echk >= 8) goto abandon_work;
	OutputStr (thread_num, "Will retry certification later.\n");
	stop_reason = STOP_RETRY_LATER;
	goto exit;

abandon_work:
	sprintf (buf, "Abandoning certification of M%lu.\n", w->n);
	OutputBoth (thread_num, buf);
	unreserve (w->n);
	goto work_complete;

work_complete:
	IniSectionWriteString (INI_FILE, SEC_Internals, KEY_CertErrorCount, NULL);
	stop_reason = STOP_WORK_UNIT_COMPLETE;
	goto exit;

exit:	gwdone (&cs.gwdata);
	return (stop_reason);
}
