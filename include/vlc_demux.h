/*****************************************************************************
 * vlc_demux.h: Demuxer descriptor, queries and methods
 *****************************************************************************
 * Copyright (C) 1999-2005 the VideoLAN team
 * $Id$
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_DEMUX_H
#define VLC_DEMUX_H 1

/**
 * \file
 * This files defines functions and structures used by demux objects in vlc
 */

#include <vlc_es.h>
#include <vlc_stream.h>
#include <vlc_es_out.h>

/**
 * \defgroup demux Demux
 * @{
 */

struct demux_t
{
    VLC_COMMON_MEMBERS

    /* Module properties */
    module_t    *p_module;

    /* eg informative but needed (we can have access+demux) */
    char        *psz_access;
    char        *psz_demux;
    char        *psz_path;

    /* input stream */
    stream_t    *s;     /* NULL in case of a access+demux in one */

    /* es output */
    es_out_t    *out;   /* our p_es_out */

    /* set by demuxer */
    int (*pf_demux)  ( demux_t * );   /* demux one frame only */
    int (*pf_control)( demux_t *, int i_query, va_list args);

    /* Demux has to maintain them uptodate
     * when it is responsible of seekpoint/title */
    struct
    {
        unsigned int i_update;  /* Demux sets them on change,
                                   Input removes them once take into account*/
        /* Seekpoint/Title at demux level */
        int          i_title;       /* idem, start from 0 (could be menu) */
        int          i_seekpoint;   /* idem, start from 0 */
    } info;
    demux_sys_t *p_sys;
};


/* demux_meta_t is returned by "meta reader" module to the demuxer */
struct demux_meta_t
{
    vlc_meta_t *p_meta;                 /**< meta data */

    int i_attachments;                  /**< number of attachments */
    input_attachment_t **attachments;    /**< array of attachments */
};

enum demux_query_e
{
    /* I. Common queries to access_demux and demux */
    /* POSITION double between 0.0 and 1.0 */
    DEMUX_GET_POSITION,         /* arg1= double *       res=    */
    DEMUX_SET_POSITION,         /* arg1= double         res=can fail    */

    /* LENGTH/TIME in microsecond, 0 if unknown */
    DEMUX_GET_LENGTH,           /* arg1= int64_t *      res=    */
    DEMUX_GET_TIME,             /* arg1= int64_t *      res=    */
    DEMUX_SET_TIME,             /* arg1= int64_t        res=can fail    */

    /* TITLE_INFO only if more than 1 title or 1 chapter */
    DEMUX_GET_TITLE_INFO,       /* arg1=input_title_t*** arg2=int*
                                   arg3=int*pi_title_offset(0), arg4=int*pi_seekpoint_offset(0) can fail */
    /* TITLE/SEEKPOINT, only when TITLE_INFO succeed */
    DEMUX_SET_TITLE,            /* arg1= int            can fail */
    DEMUX_SET_SEEKPOINT,        /* arg1= int            can fail */

    /* DEMUX_SET_GROUP only a hit for demuxer (mainly DVB) to allow not
     * reading everything (you should not use this to call es_out_Control)
     * if you don't know what to do with it, just IGNORE it, it is safe(r)
     * -1 means all group, 0 default group (first es added) */
    DEMUX_SET_GROUP,            /* arg1= int            can fail */

    /* Ask the demux to demux until the given date at the next pf_demux call
     * but not more (and not less, at the precision available of course).
     * XXX: not mandatory (except for subtitle demux) but I will help a lot
     * for multi-input
     */
    DEMUX_SET_NEXT_DEMUX_TIME,  /* arg1= int64_t *      can fail */
    /* FPS for correct subtitles handling */
    DEMUX_GET_FPS,              /* arg1= double *       res=can fail    */

    /* Meta data */
    DEMUX_GET_META,             /* arg1= vlc_meta_t **  res=can fail    */
    DEMUX_HAS_UNSUPPORTED_META, /* arg1= bool *   res can fail    */

    /* Attachments */
    DEMUX_GET_ATTACHMENTS,      /* arg1=input_attachment_t***, int* res=can fail */

    /* II. Specific access_demux queries */
    DEMUX_CAN_PAUSE,            /* arg1= bool*    can fail (assume false)*/
    DEMUX_SET_PAUSE_STATE,      /* arg1= bool     can fail */

    DEMUX_GET_PTS_DELAY,        /* arg1= int64_t*       cannot fail */

    /* DEMUX_CAN_CONTROL_PACE returns true (*pb_pace) if we can read the
     * data at our pace */
    DEMUX_CAN_CONTROL_PACE,     /* arg1= bool*pb_pace    can fail (assume false) */

    /* DEMUX_CAN_CONTROL_RATE is called only if DEMUX_CAN_CONTROL_PACE has returned false.
     * *pb_rate should be true when the rate can be changed (using DEMUX_SET_RATE)
     * *pb_ts_rescale should be true when the timestamps (pts/dts/pcr) have to be rescaled */
    DEMUX_CAN_CONTROL_RATE,     /* arg1= bool*pb_rate arg2= bool*pb_ts_rescale  can fail(assume false) */
    /* DEMUX_SET_RATE is called only if DEMUX_CAN_CONTROL_RATE has returned true.
     * It should return the value really used in *pi_rate */
    DEMUX_SET_RATE,             /* arg1= int*pi_rate                                        can fail */

    DEMUX_CAN_SEEK,            /* arg1= bool*    can fail (assume false)*/
};

VLC_EXPORT( int,       demux_vaControlHelper, ( stream_t *, int64_t i_start, int64_t i_end, int i_bitrate, int i_align, int i_query, va_list args ) );

/*************************************************************************
 * Miscellaneous helpers for demuxers
 *************************************************************************/

static inline bool demux_IsPathExtension( demux_t *p_demux, const char *psz_extension )
{
    const char *psz_ext = strrchr ( p_demux->psz_path, '.' );
    if( !psz_ext || strcasecmp( psz_ext, psz_extension ) )
        return false;
    return true;
}

static inline bool demux_IsForced( demux_t *p_demux, const char *psz_name )
{
   if( !p_demux->psz_demux || strcmp( p_demux->psz_demux, psz_name ) )
        return false;
    return true;
}

#define DEMUX_INIT_COMMON() do {            \
    p_demux->pf_control = Control;          \
    p_demux->pf_demux = Demux;              \
    MALLOC_ERR( p_demux->p_sys, demux_sys_t ); \
    memset( p_demux->p_sys, 0, sizeof( demux_sys_t ) ); } while(0)

#define STANDARD_DEMUX_INIT_MSG( msg ) do { \
    DEMUX_INIT_COMMON();                    \
    msg_Dbg( p_demux, msg ); } while(0)

#define DEMUX_BY_EXTENSION( ext ) \
    demux_t *p_demux = (demux_t *)p_this; \
    if( !demux_IsPathExtension( p_demux, ext ) ) \
        return VLC_EGENERIC; \
    DEMUX_INIT_COMMON();

#define DEMUX_BY_EXTENSION_MSG( ext, msg ) \
    demux_t *p_demux = (demux_t *)p_this; \
    if( !demux_IsPathExtension( p_demux, ext ) ) \
        return VLC_EGENERIC; \
    STANDARD_DEMUX_INIT_MSG( msg );

#define DEMUX_BY_EXTENSION_OR_FORCED( ext, module ) \
    demux_t *p_demux = (demux_t *)p_this; \
    if( !demux_IsPathExtension( p_demux, ext ) && !demux_IsForced( p_demux, module ) ) \
        return VLC_EGENERIC; \
    DEMUX_INIT_COMMON();

#define DEMUX_BY_EXTENSION_OR_FORCED_MSG( ext, module, msg ) \
    demux_t *p_demux = (demux_t *)p_this; \
    if( !demux_IsPathExtension( p_demux, ext ) && !demux_IsForced( p_demux, module ) ) \
        return VLC_EGENERIC; \
    STANDARD_DEMUX_INIT_MSG( msg );

#define CHECK_PEEK( zepeek, size ) \
    if( stream_Peek( p_demux->s , &zepeek, size ) < size ){ \
        msg_Dbg( p_demux, "not enough data" ); return VLC_EGENERIC; }

#define CHECK_PEEK_GOTO( zepeek, size ) \
    if( stream_Peek( p_demux->s , &zepeek, size ) < size ) { \
        msg_Dbg( p_demux, "not enough data" ); goto error; }

#define POKE( peek, stuff, size ) (strncasecmp( (const char *)peek, stuff, size )==0)

#define COMMON_INIT_PACKETIZER( location ) \
    location = vlc_object_create( p_demux, VLC_OBJECT_PACKETIZER ); \
    location->pf_decode_audio = 0; \
    location->pf_decode_video = 0; \
    location->pf_decode_sub = 0; \
    location->pf_packetize = 0; \

#define INIT_APACKETIZER( location, a,b,c,d ) \
    COMMON_INIT_PACKETIZER(location ); \
    es_format_Init( &location->fmt_in, AUDIO_ES, \
                    VLC_FOURCC( a, b, c, d ) );

#define INIT_VPACKETIZER( location, a,b,c,d ) \
    COMMON_INIT_PACKETIZER(location ); \
    es_format_Init( &location->fmt_in, VIDEO_ES, \
                    VLC_FOURCC( a, b, c, d ) );

/* BEWARE ! This can lead to memory leaks ! */
#define LOAD_PACKETIZER_OR_FAIL( location, msg ) \
    location->p_module = \
        module_Need( location, "packetizer", NULL, 0 ); \
    if( location->p_module == NULL ) \
    { \
        vlc_object_release( location ); \
        msg_Err( p_demux, "cannot find packetizer for " # msg ); \
        free( p_sys ); \
        return VLC_EGENERIC; \
    }

#define DESTROY_PACKETIZER( location ) \
    if( location->p_module ) module_Unneed( location, location->p_module ); \
    vlc_object_release( location );

/**
 * @}
 */

#endif
