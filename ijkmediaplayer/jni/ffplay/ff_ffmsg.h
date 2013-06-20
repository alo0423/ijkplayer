/*
 * ff_ffmsg.h
 *      based on PacketQueue in ffplay.c
 *
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FFPLAY__FF_FFMSG_H
#define FFPLAY__FF_FFMSG_H

#define FFP_MSG_FLUSH                   0
#define FFP_MSG_ERROR                   100     /* arg1 = error */
#define FFP_MSG_PREPARED                200
#define FFP_MSG_COMPLETED               300
#define FFP_MSG_VIDEO_SIZE_CHANGED      400     /* arg1 = width, arg2 = height */
#define FFP_MSG_SAR_CHANGED             401     /* arg1 = sar.num, arg2 = sar.den */
#define FFP_MSG_BUFFERING_START         500
#define FFP_MSG_BUFFERING_END           501
#define FFP_MSG_BUFFERING_FORWARD       502     /* arg1 = cached duration */
#define FFP_MSG_SEEK_COMPLETED          600

#endif
