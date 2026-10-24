// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

struct GetDataArg {
	void	*buf;
	int	bufsize;
	int	received;
	char	*MD5;		
};

size_t GetDataWriteMemoryCallback (
	void	*ptr,
	size_t	size,
	size_t	nmemb,
	void	*data)
{
	struct GetDataArg *info = (struct GetDataArg *) data;
	size_t realsize = size * nmemb;
	size_t bytes_to_copy = realsize;

	if (info->received < 32) {
		size_t len = (info->received + bytes_to_copy > 32) ? 32 - info->received : bytes_to_copy;
		memcpy ((char *) info->MD5 + info->received, ptr, len);
		info->received += (int) len;
		ptr = (char *) ptr + len;
		bytes_to_copy -= (int) len;
	}

	if (bytes_to_copy) {
		size_t	buf_received = info->received - 32;
		if ((int) buf_received + (int) bytes_to_copy <= info->bufsize) {
			memcpy ((char *) info->buf + buf_received, ptr, bytes_to_copy);
		} else {
			memcpy ((char *) info->buf + buf_received, ptr, info->bufsize - buf_received);
		}
		info->received += (int) bytes_to_copy;
	}
	return realsize;
}

int ProofGetData (char *aid, void *pbuf, int bufsize, char *md5)
{
	CURL	*curl;
	CURLcode res;
	int	try_proxy, debug;
	char	url[512], buf[600], errbuf[CURL_ERROR_SIZE];
	char	szProxyHost[PROXY_HOST_BUFSIZE];
	char	szProxyUser[PROXY_USER_BUFSIZE];
	char	szProxyPassword[PROXY_PASSWORD_BUFSIZE];
	unsigned short nProxyPort;
	float	bandwidth_rate_limit_flt = 0.0;
	uint64_t bandwidth_rate_limit;

	bandwidth_rate_limit_flt = IniSectionGetFloat (INI_FILE, SEC_PrimeNet, KEY_DownloadRateLimit, 0.0);	
	if (bandwidth_rate_limit_flt < 0.0) bandwidth_rate_limit_flt = 0.0;
	if (bandwidth_rate_limit_flt > 10000.0) bandwidth_rate_limit_flt = 10000.0;	
	bandwidth_rate_limit = (uint64_t) (bandwidth_rate_limit_flt * 131072.0);	

	debug = IniSectionGetInt (INI_FILE, SEC_PrimeNet, KEY_Debug, 0);
 
	for (try_proxy = 1; ; try_proxy = 0) {

		curl = curl_easy_init ();
		if (curl == NULL) return (PRIMENET_ERROR_CURL_INIT);
		curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1);

		sprintf (url, "http://www.mersenne.org/proof_get_data/?aid=%s", aid);
		curl_easy_setopt (curl, CURLOPT_URL, url);
		curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		if (debug) {
			sprintf (buf, "URL: %s\n", url);
			LogMsg (buf);
		}

		if (bandwidth_rate_limit)
			curl_easy_setopt (curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t) bandwidth_rate_limit);

		if (try_proxy) {
			getProxyInfo (szProxyHost, &nProxyPort, szProxyUser, szProxyPassword);
			if (szProxyHost[0]) {
				curl_easy_setopt (curl, CURLOPT_PROXY, szProxyHost);
				curl_easy_setopt (curl, CURLOPT_PROXYPORT, (long) nProxyPort);

				if (szProxyUser[0]) {
					sprintf (buf, "%s:%s", szProxyUser, szProxyPassword);
					curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, buf);
					curl_easy_setopt (curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
				}
			}
		}

		struct GetDataArg info;
		info.received = 0;
		info.buf = pbuf;
		info.bufsize = bufsize;
		info.MD5 = md5;
		memset (md5, 0, 33);
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, GetDataWriteMemoryCallback);
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
			LogMsg (buf);
			OutputStr (COMM_THREAD_NUM, buf);
			curl_easy_cleanup (curl);
			
			if (try_proxy && szProxyHost[0] && IniSectionGetInt (INI_FILE, SEC_PrimeNet, "TryNoProxyAfterProxyFailure", 1))
				continue;
			return (PRIMENET_ERROR_CURL_PERFORM);
		}

		break;
	}

	curl_easy_cleanup (curl);

	return (PRIMENET_NO_ERROR);
}

