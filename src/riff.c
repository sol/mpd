/*
 * Copyright (C) 2003-2009 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "riff.h"

#include <glib.h>

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "riff"

struct riff_header {
	char id[4];
	uint32_t size;
	char format[4];
};

struct riff_chunk_header {
	char id[4];
	uint32_t size;
};

size_t
riff_seek_id3(FILE *file)
{
	int ret;
	struct stat st;
	struct riff_header header;
	struct riff_chunk_header chunk;
	size_t size;

	/* determine the file size */

	ret = fstat(fileno(file), &st);
	if (ret < 0) {
		g_warning("Failed to stat file descriptor: %s",
			  strerror(errno));
		return 0;
	}

	/* seek to the beginning and read the RIFF header */

	ret = fseek(file, 0, SEEK_SET);
	if (ret != 0) {
		g_warning("Failed to seek: %s", g_strerror(errno));
		return 0;
	}

	size = fread(&header, sizeof(header), 1, file);
	if (size != 1 ||
	    memcmp(header.id, "RIFF", 4) != 0 ||
	    GUINT32_FROM_LE(header.size) > st.st_size)
		/* not a RIFF file */
		return 0;

	while (true) {
		/* read the chunk header */

		size = fread(&chunk, sizeof(chunk), 1, file);
		if (size != 1)
			return 0;

		size = GUINT32_FROM_LE(chunk.size);
		if (size % 2 != 0)
			/* pad byte */
			++size;

		g_debug("chunk='%.4s' size=%zu\n", chunk.id, size);

		if (memcmp(chunk.id, "id3 ", 4) == 0)
			/* found it! */
			return size;

		if ((off_t)size < 0)
			/* integer underflow after cast to signed
			   type */
			return 0;

		ret = fseek(file, size, SEEK_CUR);
		if (ret != 0)
			return 0;
	}
}