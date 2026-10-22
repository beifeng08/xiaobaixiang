// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifdef _WIN32
#include <io.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <strings.h>
#define _O_APPEND	O_APPEND
#define _O_RDONLY	O_RDONLY
#define _O_WRONLY	O_WRONLY
#define _O_RDWR		O_RDWR
#define _O_CREAT	O_CREAT
#define _O_TRUNC	O_TRUNC
#define _O_BINARY 	0
#define _O_TEXT		0
#define _open		open
#define _close		close
#define _read		read
#define _write		write
#define _unlink		unlink
#define _stricmp	strcasecmp
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "gwcommon.h"
#include "gwini.h"
#include "gwthread.h"
#include "gwutil.h"

#ifdef _WIN32
#define	CREATE_FILE_ACCESS	0600
#else
#define	CREATE_FILE_ACCESS	0666
#endif

#ifndef _O_CREAT
#define _O_APPEND	O_APPEND
#define _O_RDONLY	O_RDONLY
#define _O_WRONLY	O_WRONLY
#define _O_RDWR		O_RDWR
#define _O_CREAT	O_CREAT
#define _O_TRUNC	O_TRUNC
#define _O_BINARY 	O_BINARY
#define _O_TEXT		O_TEXT
#endif

#define INI_LINE_NORMAL		0	
#define INI_LINE_COMMENT	2	
#define INI_LINE_HEADER		3	

struct IniLine {
	char	*keyword;
	char	*value;
	int	line_type;
};

struct IniCache {
	char	*filename;
	int	immediate_writes;
	int	dirty;
	unsigned int num_lines;
	unsigned int array_size;
	struct IniLine **lines;
};

gwmutex	INI_MUTEX = NULL;		
gwmutex	INI_ADD_MUTEX = NULL;		
void (*INI_ERROR_CALLBACK)(const char *, int, const char *);	
								
struct IniCache *openIniFile (const char *, int);
void growIniLineArray (struct IniCache *);
void parse_timed_ini_value (const char *, unsigned int *, unsigned int *, unsigned int *);

void IniFileReread (			
	const char *filename)
{
	struct IniCache *p;

	if (INI_MUTEX == NULL) gwmutex_init (&INI_MUTEX);
	gwmutex_lock (&INI_MUTEX);
	p = openIniFile (filename, 1);
	gwmutex_unlock (&INI_MUTEX);
}

void writeIniFile (			
	struct IniCache *p)
{
	int	fd;
	unsigned int j;
	char	buf[2000];

	if (!p->immediate_writes) {
		p->dirty = 1;
		return;
	}

	fd = _open (p->filename, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT, CREATE_FILE_ACCESS);
	if (fd < 0) return;
	for (j = 0; j < p->num_lines; j++) {
		if (p->lines[j]->line_type == INI_LINE_COMMENT) {
			strcpy (buf, p->lines[j]->value);
		} else if (p->lines[j]->line_type == INI_LINE_HEADER) {
			strcpy (buf, p->lines[j]->value);
		} else {
			strcpy (buf, p->lines[j]->keyword);
			strcat (buf, "=");
			strcat (buf, p->lines[j]->value);
		}
		strcat (buf, "\n");
		(void) _write (fd, buf, (unsigned int) strlen (buf));
	}
	p->dirty = 0;
	_close (fd);
}

void IniDelayWrites (
	const char *filename)
{
	struct IniCache *p;
	p = openIniFile (filename, 0);
	p->immediate_writes = FALSE;
}

void IniResumeImmediateWrites (
	const char *filename)
{
	struct IniCache *p;
	p = openIniFile (filename, 0);
	p->immediate_writes = TRUE;
	if (p->dirty) writeIniFile (p);
}

void IniAddFileMerge (
	const char *ini_filename,
	const char *add_filename,
	const char *section_to_copy_to)
{
	struct IniCache *p, *q;
	const char *section;
	unsigned int j;

	if (INI_ADD_MUTEX == NULL) gwmutex_init (&INI_ADD_MUTEX);
	gwmutex_lock (&INI_ADD_MUTEX);

	p = openIniFile (ini_filename, 0);
	q = openIniFile (add_filename, 1);

	p->immediate_writes = FALSE;

	section = section_to_copy_to;
	for (j = 0; j < q->num_lines; j++) {
		if (q->lines[j]->line_type == INI_LINE_HEADER) {
			if (section_to_copy_to == NULL)
				section = q->lines[j]->keyword;
		}
		else if (q->lines[j]->line_type != INI_LINE_COMMENT)
			IniSectionWriteString (ini_filename, section, q->lines[j]->keyword, q->lines[j]->value);
	}

	p->immediate_writes = TRUE;
	writeIniFile (p);

	_unlink (add_filename);

	gwmutex_unlock (&INI_ADD_MUTEX);
}

void IniGetString (			
	const char *filename,
	const char *keyword,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val)
{
	IniSectionGetString (filename, NULL, keyword, val, val_bufsize, default_val);
}

void IniSectionGetString (		
	const char *filename,
	const char *section,
	const char *keyword,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val)
{
	IniSectionGetNthString (filename, section, keyword, 1, val, val_bufsize, default_val);
}

void IniGetNthString (			
	const char *filename,
	const char *keyword,
	int	nth,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val)
{
	IniSectionGetNthString (filename, NULL, keyword, nth, val, val_bufsize, default_val);
}

void IniSectionGetNthString (		
	const char *filename,
	const char *section,
	const char *keyword,
	int	nth,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val)
{
	unsigned int seconds;
	IniSectionGetNthTimedString (filename, section, keyword, nth, val, val_bufsize, default_val, &seconds);
}

void IniGetTimedString (		
	const char *filename,
	const char *keyword,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val,
	unsigned int *seconds)		
{
	IniSectionGetTimedString (filename, NULL, keyword, val, val_bufsize, default_val, seconds);
}

void IniSectionGetTimedString (		
	const char *filename,
	const char *section,
	const char *keyword,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val,
	unsigned int *seconds)		
{
	IniSectionGetNthTimedString (filename, section, keyword, 1, val, val_bufsize, default_val, seconds);
}

void IniSectionGetNthTimedString (	
	const char *filename,
	const char *section,
	const char *keyword,
	int	nth,
	char	*val,
	unsigned int val_bufsize,
	const char *default_val,
	unsigned int *seconds)		
{
	const char *p;
	unsigned int start, len;

	p = IniSectionGetNthStringRaw (filename, section, keyword, nth);

	if (p != NULL) {
		parse_timed_ini_value (p, &start, &len, seconds);
		if (len) {
			truncated_strcpy_with_len (val, val_bufsize, p+start, len);
			return;
		}
	} else {
		*seconds = 0;
	}

	if (default_val)
		truncated_strcpy (val, val_bufsize, default_val);
	else
		val[0] = 0;
}

const char *IniSectionGetStringRaw (	
	const char *filename,
	const char *section,
	const char *keyword)
{
	return (IniSectionGetNthStringRaw (filename, section, keyword, 1));
}

const char *IniSectionGetNthStringRaw (	
	const char *filename,
	const char *section,
	const char *keyword,
	int	nth)			
{
	struct IniCache *p;
	unsigned int i;
	const char *retval;

	if (INI_MUTEX == NULL) gwmutex_init (&INI_MUTEX);
	gwmutex_lock (&INI_MUTEX);
	p = openIniFile (filename, 0);

	i = 0;
	if (section != NULL) {
		for ( ; i < p->num_lines; i++) {
			if (p->lines[i]->line_type == INI_LINE_HEADER &&
			    _stricmp (section, p->lines[i]->keyword) == 0) {
				i++;
				break;
			}
		}
	}

	for ( ; ; i++) {
		if (i == p->num_lines ||
		    p->lines[i]->line_type == INI_LINE_HEADER) {
			retval = NULL;
			break;
		}
		if (p->lines[i]->line_type == INI_LINE_NORMAL &&
		    _stricmp (keyword, p->lines[i]->keyword) == 0 &&
		    --nth == 0) {
			retval = p->lines[i]->value;
			break;
		}
	}

	gwmutex_unlock (&INI_MUTEX);
	return (retval);
}

void IniWriteString (			
	const char *filename,
	const char *keyword,
	const char *val)
{
	IniSectionWriteString (filename, NULL, keyword, val);
}

void IniSectionWriteString (		
	const char *filename,
	const char *section,
	const char *keyword,
	const char *val)
{
	IniSectionWriteNthString (filename, section, keyword, 1, val);
}

void IniWriteNthString (		
	const char *filename,
	const char *keyword,
	int	nth,
	const char *val)		
{
	IniSectionWriteNthString (filename, NULL, keyword, nth, val);
}

void IniSectionWriteNthString (		
	const char *filename,
	const char *section,
	const char *keyword,		
	int	nth,
	const char *val)		
{
	struct IniCache *p;
	unsigned int i, j, insertion_point;
	int	lines_were_deleted;

	if (INI_MUTEX == NULL) gwmutex_init (&INI_MUTEX);
	gwmutex_lock (&INI_MUTEX);
	p = openIniFile (filename, 0);

	i = 0;
	if (section != NULL) {
		for ( ; ; i++) {
			if (i == p->num_lines) {
				if (val == NULL) goto nowrite_done;
				growIniLineArray (p);
				p->lines[i] = (struct IniLine *) malloc (sizeof (struct IniLine));
				p->lines[i]->line_type = INI_LINE_COMMENT;
				p->lines[i]->keyword = NULL;
				p->lines[i]->value = (char *) malloc (1);
				p->lines[i]->value[0] = 0;
				p->num_lines++;
				i++;
				growIniLineArray (p);
				p->lines[i] = (struct IniLine *) malloc (sizeof (struct IniLine));
				p->lines[i]->line_type = INI_LINE_HEADER;
				p->lines[i]->keyword = (char *) malloc (strlen (section) + 1);
				strcpy (p->lines[i]->keyword, section);
				p->lines[i]->value = (char *) malloc (strlen (section) + 3);
				sprintf (p->lines[i]->value, "[%s]", section);
				p->num_lines++;
				i++;
				break;
			}
			if (p->lines[i]->line_type == INI_LINE_HEADER && _stricmp (section, p->lines[i]->keyword) == 0) {
				i++;
				break;
			}
		}
	}

	insertion_point = i;
	lines_were_deleted = FALSE;
	for ( ; ; i++) {
		if (i == p->num_lines || p->lines[i]->line_type == INI_LINE_HEADER ||
		    (p->lines[i]->line_type != INI_LINE_COMMENT && _stricmp (p->lines[i]->keyword, "Time") == 0)) {

			if (val == NULL) {
				if (lines_were_deleted) goto write_done;
				goto nowrite_done;
			}

			growIniLineArray (p);

			i = insertion_point;
			for (j = p->num_lines; j > i; j--) p->lines[j] = p->lines[j-1];

			p->lines[i] = (struct IniLine *) malloc (sizeof (struct IniLine));
			if (keyword == NULL) {
				p->lines[i]->line_type = INI_LINE_COMMENT;
				p->lines[i]->keyword = NULL;
			} else {
				p->lines[i]->line_type = INI_LINE_NORMAL;
				p->lines[i]->keyword = (char *) malloc (strlen (keyword) + 1);
				strcpy (p->lines[i]->keyword, keyword);
			}
			p->lines[i]->value = NULL;
			p->num_lines++;
			break;
		}

		if (p->lines[i]->line_type != INI_LINE_COMMENT || p->lines[i]->value[0]) {
			insertion_point = i + 1;
		}

		if (p->lines[i]->line_type == INI_LINE_NORMAL && keyword != NULL && _stricmp (keyword, p->lines[i]->keyword) == 0) {

			if (val == NULL) {

				free (p->lines[i]->keyword);
				free (p->lines[i]->value);
				free (p->lines[i]);

				for (j = i + 1; j < p->num_lines; j++) p->lines[j-1] = p->lines[j];
				p->num_lines--;
				i--;
				lines_were_deleted = TRUE;
			}

			else if (--nth == 0) {
				if (strcmp (val, p->lines[i]->value) == 0) goto nowrite_done;
				break;
			}
		}
	}

	if (val != NULL) {
		free (p->lines[i]->value);
		p->lines[i]->value = (char *) malloc (strlen (val) + 1);
		strcpy (p->lines[i]->value, val);
	}

write_done:
	writeIniFile (p);

nowrite_done:
	gwmutex_unlock (&INI_MUTEX);
}

long IniGetInt (			
	const char *filename,
	const char *keyword,
	long	default_val)
{
	return (IniSectionGetInt (filename, NULL, keyword, default_val));
}

long IniSectionGetInt (			
	const char *filename,
	const char *section,
	const char *keyword,
	long	default_val)
{
	unsigned int seconds;
	return (IniSectionGetTimedInt (filename, section, keyword, default_val, &seconds));
}

long IniGetTimedInt (			
	const char *filename,
	const char *keyword,
	long	default_val,
	unsigned int *seconds)		
{
	return (IniSectionGetTimedInt (filename, NULL, keyword, default_val, seconds));
}

long IniSectionGetTimedInt (		
	const char *filename,
	const char *section,
	const char *keyword,
	long	default_val,
	unsigned int *seconds)		
{
	char	buf[20], defval[20];
	sprintf (defval, "%ld", default_val);
	IniSectionGetTimedString (filename, section, keyword, buf, 20, defval, seconds);
	return (atol (buf));
}

void IniWriteInt (			
	const char *filename,
	const char *keyword,
	long	val)
{
	IniSectionWriteInt (filename, NULL, keyword, val);
}

void IniSectionWriteInt (		
	const char *filename,
	const char *section,
	const char *keyword,
	long	val)
{
	char	buf[20];
	sprintf (buf, "%ld", val);
	IniSectionWriteString (filename, section, keyword, buf);
}

int64_t IniGetInt64 (			
	const char *filename,
	const char *keyword,
	int64_t	default_val)
{
	return (IniSectionGetInt64 (filename, NULL, keyword, default_val));
}

int64_t IniSectionGetInt64 (		
	const char *filename,
	const char *section,
	const char *keyword,
	int64_t	default_val)
{
	unsigned int seconds;
	return (IniSectionGetTimedInt64 (filename, section, keyword, default_val, &seconds));
}

int64_t IniGetTimedInt64 (		
	const char *filename,
	const char *keyword,
	int64_t	default_val,
	unsigned int *seconds)		
{
	return (IniSectionGetTimedInt64 (filename, NULL, keyword, default_val, seconds));
}

int64_t IniSectionGetTimedInt64 (	
	const char *filename,
	const char *section,
	const char *keyword,
	int64_t	default_val,
	unsigned int *seconds)		
{
	char	buf[30], defval[30];
	sprintf (defval, "%" PRIi64, default_val);
	IniSectionGetTimedString (filename, section, keyword, buf, 30, defval, seconds);
	return (atoll (buf));
}

void IniWriteInt64 (			
	const char *filename,
	const char *keyword,
	int64_t	val)
{
	IniSectionWriteInt64 (filename, NULL, keyword, val);
}

void IniSectionWriteInt64 (		
	const char *filename,
	const char *section,
	const char *keyword,
	int64_t	val)
{
	char	buf[30];
	sprintf (buf, "%" PRIi64, val);
	IniSectionWriteString (filename, section, keyword, buf);
}

float IniGetFloat (			
	const char *filename,
	const char *keyword,
	float	default_val)
{
	return (IniSectionGetFloat (filename, NULL, keyword, default_val));
}

float IniSectionGetFloat (		
	const char *filename,
	const char *section,
	const char *keyword,
	float	default_val)
{
	unsigned int seconds;
	return (IniSectionGetTimedFloat (filename, section, keyword, default_val, &seconds));
}

float IniGetTimedFloat (		
	const char *filename,
	const char *keyword,
	float	default_val,
	unsigned int *seconds)		
{
	return (IniSectionGetTimedFloat (filename, NULL, keyword, default_val, seconds));
}

float IniSectionGetTimedFloat (		
	const char *filename,
	const char *section,
	const char *keyword,
	float	default_val,
	unsigned int *seconds)		
{
	char	buf[20], defval[20];
	sprintf (defval, "%f", default_val);
	IniSectionGetTimedString (filename, section, keyword, buf, 20, defval, seconds);
	return ((float) atof (buf));
}

void IniWriteFloat (			
	const char *filename,
	const char *keyword,
	float	val)
{
	IniSectionWriteFloat (filename, NULL, keyword, val);
}

void IniSectionWriteFloat (		
	const char *filename,
	const char *section,
	const char *keyword,
	float	val)
{
	
 	char	buf[20];
	sprintf (buf, "%g", val);
 	IniSectionWriteString (filename, section, keyword, buf);
}

struct IniCache *openIniFile (
	const char *filename,
	int	forced_read)
{
static	struct IniCache *cache[10] = {0};
	struct IniCache *p;
	FILE	*fd;
	unsigned int i;
	char	line[1024];
	char	*val;

	for (i = 0; i < 10; i++) {
		p = cache[i];
		if (p == NULL) {
			p = (struct IniCache *) malloc (sizeof (struct IniCache));
			p->filename = (char *) malloc (strlen (filename) + 1);
			strcpy (p->filename, filename);
			p->immediate_writes = 1;
			p->dirty = 0;
			p->num_lines = 0;
			p->array_size = 0;
			p->lines = NULL;
			forced_read = 1;
			cache[i] = p;
			break;
		}
		if (strcmp (filename, p->filename) == 0)
			break;
	}

	if (!forced_read) return (p);
	if (p->dirty) return (p);

	for (i = 0; i < p->num_lines; i++) {
		free (p->lines[i]->keyword);
		free (p->lines[i]->value);
		free (p->lines[i]);
	}
	p->num_lines = 0;

	fd = fopen (filename, "r");
	if (fd == NULL) return (p);

	while (fgets (line, sizeof (line), fd)) {
		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
		if (line[0] && line[strlen(line)-1] == '\r') line[strlen(line)-1] = 0;

		growIniLineArray (p);
		i = p->num_lines++;
		p->lines[i] = (struct IniLine *) malloc (sizeof (struct IniLine));

		if (line[0] == '[') {
			char	*q;

			p->lines[i]->keyword = (char *) malloc (strlen (line) + 1);
			p->lines[i]->value = (char *) malloc (strlen (line) + 1);
			p->lines[i]->line_type = INI_LINE_HEADER;
			strcpy (p->lines[i]->value, line);
			strcpy (p->lines[i]->keyword, line+1);
			for (q = p->lines[i]->keyword; *q; q++)
				if (*q == ']') {
					*q = 0;
					break;
				}
		}

		else if ((line[0] < 'A' || line[0] > 'Z') &&
			 (line[0] < 'a' || line[0] > 'z')) {
			p->lines[i]->keyword = NULL;
			p->lines[i]->value = (char *) malloc (strlen (line) + 1);
			p->lines[i]->line_type = INI_LINE_COMMENT;
			strcpy (p->lines[i]->value, line);
		}

		else {
			val = strchr (line, '=');
			if (val == NULL) {
				
				if (INI_ERROR_CALLBACK != NULL) (*INI_ERROR_CALLBACK)(filename, p->num_lines, line);
				
				p->lines[i]->keyword = NULL;
				p->lines[i]->value = (char *) malloc (strlen (line) + 1);
				p->lines[i]->line_type = INI_LINE_COMMENT;
				strcpy (p->lines[i]->value, line);
			} else {
				*val++ = 0;
				p->lines[i]->keyword = (char *) malloc (strlen (line) + 1);
				p->lines[i]->value = (char *) malloc (strlen (val) + 1);
				p->lines[i]->line_type = INI_LINE_NORMAL;
				strcpy (p->lines[i]->keyword, line);
				strcpy (p->lines[i]->value, val);
			}
		}
	}
	fclose (fd);

	return (p);
}

void growIniLineArray (
	struct IniCache *p)
{
	struct IniLine **newlines;

	if (p->num_lines != p->array_size) return;

	newlines = (struct IniLine **) malloc ((p->num_lines + 100) * sizeof (struct IniLine *));
	if (p->num_lines) {
		memcpy (newlines, p->lines, p->num_lines * sizeof (struct IniLine *));
		free (p->lines);
	}
	p->lines = newlines;
	p->array_size = p->num_lines + 100;
}

void parseTimeLine (
	const char **line,
	int	*start_day,
	int	*end_day,
	int	*start_time,
	int	*end_time)
{
	const char *p;

	p = *line;
	*start_day = atoi (p); while (isdigit (*p)) p++;
	if (*p == '-') {
		p++;
		*end_day = atoi (p); while (isdigit (*p)) p++;
	} else
		*end_day = *start_day;

	if (*p == '/')
		p++;
	else {
		p = *line;
		*start_day = 1;
		*end_day = 7;
	} 
	*start_time = atoi (p) * 60; while (isdigit (*p)) p++;
	if (*p == ':') {
		p++;
		*start_time += atoi (p); while (isdigit (*p)) p++;
	}
	if (*p == '-') p++;			
	*end_time = atoi (p) * 60; while (isdigit (*p)) p++;
	if (*p == ':') {
		p++;
		*end_time += atoi (p); while (isdigit (*p)) p++;
	}

	if (*p++ == ',') *line = p;
	else *line = NULL;
}

int analyzeTimeLine (
	const char *line,
	time_t	current_t,
	unsigned int *wakeup_time)
{
	struct tm *x;
	int	current_time;
	const char *p;
	int	day, start_day, end_day, start_time, end_time;
	int	full_start_time, full_end_time;
	int	wakeup_t, min_wakeup_t;

	x = localtime (&current_t);
	current_time = (x->tm_wday ? x->tm_wday : 7) * 24 * 60;
	current_time += x->tm_hour * 60 + x->tm_min;

	p = line;
	min_wakeup_t = 0;
	while (p != NULL) {
		parseTimeLine (&p, &start_day, &end_day, &start_time, &end_time);

		for (day = start_day; day <= end_day; day++) {

			full_start_time = day * 24 * 60 + start_time;
			full_end_time = day * 24 * 60 + end_time;
			if (end_time < start_time) full_end_time += 24 * 60;

			if (current_time >= full_start_time &&
			    current_time < full_end_time)
				goto winner;

			if (end_time < start_time && day == 7 &&
			    current_time < full_end_time - 7 * 24 * 60)
				goto winner;

			if (full_start_time >= current_time)
				wakeup_t = (full_start_time - current_time) * 60;
			else
				wakeup_t = (full_start_time + 7 * 24 * 60 - current_time) * 60;
			if (min_wakeup_t == 0 || min_wakeup_t > wakeup_t)
				min_wakeup_t = wakeup_t;
		}
	}

	*wakeup_time = min_wakeup_t;
	return (FALSE);

winner:	wakeup_t = (full_end_time - current_time) * 60;

	p = line;
	while (p != NULL && wakeup_t < 10 * 24 * 60) {
		parseTimeLine (&p, &start_day, &end_day, &start_time, &end_time);

		for (day = start_day; day <= end_day; day++) {
			int	this_full_start_time, this_full_end_time;

			this_full_start_time = day * 24 * 60 + start_time;
			if (this_full_start_time != full_end_time &&
			    this_full_start_time != full_end_time - 7 * 24 * 60) continue;

			this_full_end_time = day * 24 * 60 + end_time;
			if (end_time < start_time) this_full_end_time += 24 * 60;
			wakeup_t += (this_full_end_time - this_full_start_time) * 60;
			full_end_time = this_full_end_time;
			p = line;
			break;
		}
	}

	*wakeup_time = wakeup_t + 1;
	return (TRUE);
}

void parse_timed_ini_value (
	const char *line,		
	unsigned int *start_offset,	
	unsigned int *len,		
	unsigned int *seconds_valid)	
					
{
	time_t	current_time;
	const char *rest_of_line, *during_clause, *else_clause;
	unsigned int min_wakeup_time, wakeup_time;

	time (&current_time);

	rest_of_line = line;
	min_wakeup_time = 0;
	for ( ; ; ) {

		during_clause = strstr (rest_of_line, " during ");
		if (during_clause == NULL) {
			*start_offset = (unsigned int) (rest_of_line - line);
			*len = (unsigned int) strlen (rest_of_line);
			*seconds_valid = min_wakeup_time;
			break;
		}

		else_clause = strstr (rest_of_line, " else ");
		if (else_clause != NULL &&
		    during_clause - rest_of_line == strlen (else_clause + 6) &&
		    memcmp (rest_of_line, else_clause + 6, during_clause - rest_of_line) == 0) {
			*start_offset = (unsigned int) (rest_of_line - line);
			*len = (unsigned int) (during_clause - rest_of_line);
			*seconds_valid = min_wakeup_time;
			break;
		}

		if (analyzeTimeLine (during_clause+8, current_time, &wakeup_time)) {
			*start_offset = (unsigned int) (rest_of_line - line);
			*len = (unsigned int) (during_clause - rest_of_line);
			*seconds_valid = wakeup_time;
			break;
		}

		if (min_wakeup_time == 0 || wakeup_time < min_wakeup_time)
			min_wakeup_time = wakeup_time;

		if (else_clause != NULL) rest_of_line = else_clause + 6;
		else rest_of_line += strlen (rest_of_line);
	}
}

