/*--------------------------------------------------------------------------
| Copyright 2020-2023 Mersenne Research, Inc.  All rights reserved
|
| This file contains routines to upload one proof file to the Primenet server
+--------------------------------------------------------------------------*/

#include "cJSON.h"

void archiveOrDelete (FILE **fd, char *filename, char fileMD5[33])
{
	char	archive_filename[512];

	IniGetString (INI_FILE, "ProofArchiveDir", archive_filename, sizeof (archive_filename), NULL);

	if (archive_filename[0]) {
		FILE	*fdout;
		char	buf[16384];
		int	bytes_read;
		char	archiveMD5[33];

		DirPlusFilename (archive_filename, filename);
		fdout = fopen (archive_filename, "wb");
		if (fdout == NULL) {
			sprintf (buf, "Unable to create archive proof file %s\n", archive_filename);
			OutputBoth (COMM_THREAD_NUM, buf);
			return;
		}

		fseek (*fd, 0, SEEK_SET);
		while ((bytes_read = (int) fread (buf, 1, sizeof (buf), *fd)) > 0) {
			fwrite (buf, 1, bytes_read, fdout);
		}

		fclose (fdout);
		md5_hexdigest_file (archiveMD5, archive_filename);
		if (strcmp (fileMD5, archiveMD5) != 0) {
			sprintf (buf, "Error copy proof file to archive %s\n", archive_filename);
			OutputBoth (COMM_THREAD_NUM, buf);
			return;
		}
	}

	fclose (*fd);
	*fd = NULL;
	_unlink (filename);
}

struct UploadArg {
	void	*buf;
	int	bufsize;
	int	received;
};

size_t UploadWriteMemoryCallback (
	void	*ptr,
	size_t	size,
	size_t	nmemb,
	void	*data)
{
	size_t realsize = size * nmemb;
	struct UploadArg *info = (struct UploadArg *) data;

	if (info->received + (int) realsize <= info->bufsize) {
		memcpy ((char *) info->buf + info->received, ptr, realsize);
	} else {
		memcpy ((char *) info->buf + info->received, ptr, info->bufsize - info->received);
	}
	info->received += (int) realsize;
	((char *)info->buf)[info->received] = 0;
	return realsize;
}

size_t UploadReadMemoryCallback (
	void	*ptr,
	size_t	size,
	size_t	nmemb,
	void	*data)
{
	size_t realsize = size * nmemb;
	struct UploadArg *info = (struct UploadArg *) data;

	if (info->received + (int) realsize <= info->bufsize) {
		memcpy ((char *) info->buf + info->received, ptr, realsize);
	} else {
		memcpy ((char *) info->buf + info->received, ptr, info->bufsize - info->received);
	}
	info->received += (int) realsize;
	((char *)info->buf)[info->received] = 0;
	return realsize;
}

void ProofUpload (char *filename)
{
	FILE	*fd = NULL;
	int	exponent, version, power, power_mult, prp_base, hashlen;
	char	number[2048], newline[2], fileMD5[33], buf[4096];
	uint64_t filesize;
	float	max_chunk_size_flt = 5.0;
	float	bandwidth_rate_limit_flt = 0.25;
	int	max_chunk_size;
	uint64_t bandwidth_rate_limit;
	char	*chunk = NULL;
	CURL	*curl = NULL;
	curl_mime *mime = NULL;
	cJSON	*json = NULL;

	bandwidth_rate_limit_flt = IniSectionGetFloat (INI_FILE, SEC_PrimeNet, KEY_UploadRateLimit, 0.25);	
	if (bandwidth_rate_limit_flt < 0.0) bandwidth_rate_limit_flt = 0.0;
	if (bandwidth_rate_limit_flt > 10000.0) bandwidth_rate_limit_flt = 10000.0;	

	if (bandwidth_rate_limit_flt == 0.0) max_chunk_size_flt = 7.0;			
	else if (bandwidth_rate_limit_flt >= 8.0) max_chunk_size_flt = 4.0;
	else if (bandwidth_rate_limit_flt >= 1.0) max_chunk_size_flt = 2.0;
	else max_chunk_size_flt = 1.0;
	max_chunk_size_flt = IniSectionGetFloat (INI_FILE, SEC_PrimeNet, KEY_UploadChunkSize, max_chunk_size_flt);
	if (max_chunk_size_flt <= 1.0) max_chunk_size_flt = 1.0;
	if (max_chunk_size_flt >= 8.0) max_chunk_size_flt = 8.0;			

	max_chunk_size = (int) (max_chunk_size_flt * 1048576.0);			
	bandwidth_rate_limit = (uint64_t) (bandwidth_rate_limit_flt * 131072.0);	

	fd = fopen (filename, "rb");
	if (fd == NULL) {
		sprintf (buf, "Cannot open proof file %s\n", filename);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}
	fclose (fd);
	fd = NULL;

	md5_hexdigest_file (fileMD5, filename);
	sprintf (buf, "MD5 of %s is %s\n", filename, fileMD5);
	OutputStr (COMM_THREAD_NUM, buf);

	fd = fopen (filename, "rb");
	if (fd == NULL) {
		sprintf (buf, "Cannot open proof file %s\n", filename);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}
	fscanf (fd, "PRP PROOF\n");
	if (fscanf (fd, "VERSION=%d\n", &version) != 1 || (version != 1 && version != 2)) {
		OutputBoth (COMM_THREAD_NUM, "Error getting version number from proof header\n");
		goto end;
	}
	if (fscanf (fd, "HASHSIZE=%d\n", &hashlen) != 1 || hashlen < 32 || hashlen > 64) {
		OutputBoth (COMM_THREAD_NUM, "Error getting hash size from proof header\n");
		goto end;
	}
	if (fscanf (fd, "POWER=%d\n", &power) != 1 || power <= 0 || power >= 16) {
		OutputBoth (COMM_THREAD_NUM, "Error getting power from proof header\n");
		goto end;
	}
	if (fscanf (fd, "x%d\n", &power_mult) != 1) power_mult = 1;		
	if (fscanf (fd, "BASE=%d\n", &prp_base) != 1) prp_base = 3;		
	if (fscanf (fd, "NUMBER=%2047[^\n]%1[\n]", number, newline) != 2 || number[0] != 'M') {
		OutputBoth (COMM_THREAD_NUM, "Error getting number from proof header\n");
		goto end;
	}

	exponent = atoi (&number[1]);
	sprintf (buf, "Proof file exponent is %d\n", exponent);
	OutputStr (COMM_THREAD_NUM, buf);

	fseek (fd, 0, SEEK_END);
	filesize = ftell (fd);
	fseek (fd, 0, SEEK_SET);
	sprintf (buf, "Filesize of %s is %" PRIu64 "\n", filename, filesize);
	OutputStr (COMM_THREAD_NUM, buf);

	CURLcode res;
	int	debug;
	char	url[512], curlbuf[3000], errbuf[CURL_ERROR_SIZE];

	debug = IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_Debug, 0);
 
	curl = curl_easy_init ();
	if (curl == NULL) {
		OutputStr (COMM_THREAD_NUM, "curl_easy_init failed\n");
		goto end;
	}
	curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1);

	sprintf (url, "http://www.mersenne.org/proof_upload/?UserID=%s&Exponent=%d&FileSize=%" PRIu64 "&FileMD5=%s", USERID, exponent, filesize, fileMD5);
	curl_easy_setopt (curl, CURLOPT_URL, url);
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	if (debug) {
		sprintf (buf, "URL: %s\n", url);
		LogMsg (buf);
	}

	curlbuf[0] = 0;
	struct UploadArg info;
	info.received = 0;
	info.buf = curlbuf;
	info.bufsize = sizeof (curlbuf) - 1;
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, UploadWriteMemoryCallback);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &info);

	if (debug >= 2) {
		curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION, curl_trace);
		curl_easy_setopt (curl, CURLOPT_DEBUGDATA, NULL);
		curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
	}

	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 180);
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, 180);
	curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errbuf);
	res = curl_easy_perform (curl);
	if (res != CURLE_OK) {
		sprintf (buf, "CURL library error: %s\n", errbuf);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}

	cJSON	*item;
	char	base_url[512], chunkMD5[33];
	uint64_t chunk_start, chunk_end;

	json = cJSON_Parse (curlbuf);

	item = cJSON_GetObjectItem (json, "error_status");
	if (item != NULL) {
		int error_code = (int) cJSON_GetNumberValue (item);

		if (error_code == 409) {
			sprintf (buf, "Proof %s already uploaded (%s)\n", filename, curlbuf);
			archiveOrDelete (&fd, filename, fileMD5);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}

		sprintf (buf, "Unexpected error during %s upload: %s\n", filename, curlbuf);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}

	item = cJSON_GetObjectItem (json, "URLToUse");
	if (item == NULL) {
		sprintf (buf, "For proof %s, server response missing URLToUse: %s\n", filename, curlbuf);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}
	strcpy (base_url, cJSON_GetStringValue (item));

	item = cJSON_GetObjectItem (json, "need");
	if (item == NULL) {
		sprintf (buf, "For proof %s, server response missing need list: %s\n", filename, curlbuf);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}
	item = cJSON_GetArrayItem (item, 0);
	if (sscanf (item->string, "%" PRIu64, &chunk_start) != 1) {
		sprintf (buf, "For proof %s, error parsing first need list entry: %s\n", filename, curlbuf);
		goto end;
	}
	chunk_end = (uint64_t) cJSON_GetNumberValue(item);
	if (chunk_start > chunk_end || chunk_end >= filesize) {
		printf ("For proof %s, need list entry bad: %s\n", filename, curlbuf);
		OutputBoth (COMM_THREAD_NUM, buf);
		goto end;
	}

	cJSON_Delete (json);
	json = NULL;

	if (chunk_start) {
		sprintf (buf, "Resuming from offset %" PRIu64 "\n", chunk_start);
		OutputStr (COMM_THREAD_NUM, buf);
	}

	chunk = (char *) malloc (max_chunk_size);
	if (chunk == NULL) {
		OutputStr (COMM_THREAD_NUM, "Error allocating upload buffer\n");
		goto end;
	}

	for ( ; ; ) {
		int	datasize;

		if (!inUploadWindow (NULL)) {
			OutputStr (COMM_THREAD_NUM, "Proof file upload suspended\n");
			goto end;
		}

		_fseeki64 (fd, chunk_start, SEEK_SET);
		datasize = (int) (chunk_end - chunk_start + 1);
		if (datasize > max_chunk_size) datasize = max_chunk_size;
		datasize = (int) fread (chunk, 1, datasize, fd);

		if (datasize <= 0) {
			sprintf (buf, "Error reading proof file %s\n", filename);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}

		md5_hexdigest_buffer (chunkMD5, chunk, datasize);

		sprintf (url, "%s&FileMD5=%s&DataOffset=%" PRIu64 "&DataSize=%d&DataMD5=%s", base_url, fileMD5, chunk_start, datasize, chunkMD5);
		curl_easy_setopt (curl, CURLOPT_URL, url);
		curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		if (debug) {
			sprintf (buf, "URL: %s\n", url);
			LogMsg (buf);
		}

		curl_mimepart *part;
		
		mime = curl_mime_init (curl);
		part = curl_mime_addpart (mime);
		curl_mime_name (part, "Data");
		curl_mime_data (part, chunk, datasize);
		curl_easy_setopt (curl, CURLOPT_MIMEPOST, mime);

		if (bandwidth_rate_limit)
			curl_easy_setopt (curl, CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t) bandwidth_rate_limit);

		curlbuf[0] = 0;
		info.received = 0;
		info.buf = curlbuf;
		info.bufsize = sizeof (curlbuf) - 1;
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, UploadWriteMemoryCallback);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &info);

		if (debug >= 2) {
			curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION, curl_trace);
			curl_easy_setopt (curl, CURLOPT_DEBUGDATA, NULL);
			curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
		}

		curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 180);
		curl_easy_setopt (curl, CURLOPT_TIMEOUT, 180);
		curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errbuf);
		res = curl_easy_perform (curl);
		if (res != CURLE_OK) {
			sprintf (buf, "CURL library error: %s\n", errbuf);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}
		curl_mime_free (mime);
		mime = NULL;

		json = cJSON_Parse (curlbuf);

		item = cJSON_GetObjectItem (json, "error_status");
		if (item != NULL) {
			sprintf (buf, "Unexpected error during %s upload: %s\n", filename, curlbuf);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}

		item = cJSON_GetObjectItem(json, "FileUploaded");
		if (item != NULL) {
			sprintf (buf, "Proof file %s successfully uploaded\n", filename);
			LogMsg (buf);
			writeResults (buf);
			archiveOrDelete (&fd, filename, fileMD5);
			goto end;
		}

		item = cJSON_GetObjectItem (json, "need");
		if (item == NULL) {
			sprintf (buf, "For proof %s, no entries in need list: %s\n", filename, curlbuf);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}
		item = cJSON_GetArrayItem (item, 0);
		{
			uint64_t new_chunk_start;
			if (sscanf (item->string, "%" PRIu64, &new_chunk_start) != 1) {
				sprintf (buf, "For proof %s, error parsing first need list entry: %s\n", filename, curlbuf);
				OutputBoth (COMM_THREAD_NUM, buf);
				goto end;
			}
			if (new_chunk_start <= chunk_start) {
				sprintf (buf, "For proof %s, sending data did not advance need list: %s\n", filename, curlbuf);
				OutputBoth (COMM_THREAD_NUM, buf);
				goto end;
			}
			chunk_start = new_chunk_start;
		}
		chunk_end = (uint64_t) cJSON_GetNumberValue (item);
		if (chunk_start > chunk_end || chunk_end >= filesize) {
			sprintf (buf, "For proof %s, need list entry bad: %s\n", filename, curlbuf);
			OutputBoth (COMM_THREAD_NUM, buf);
			goto end;
		}

		cJSON_Delete (json);
		json = NULL;
	}

end:	if (mime != NULL) curl_mime_free (mime);
	if (curl != NULL) curl_easy_cleanup (curl);
	if (fd != NULL) fclose (fd);
	if (json != NULL) cJSON_Delete (json);
	if (chunk != NULL) free (chunk);
}
