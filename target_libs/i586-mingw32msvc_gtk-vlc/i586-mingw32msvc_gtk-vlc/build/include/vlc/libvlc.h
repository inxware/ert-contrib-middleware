/*****************************************************************************
 * libvlc.h:  libvlc external API
 *****************************************************************************
 * Copyright (C) 1998-2009 the VideoLAN team
 * $Id: 614c3b0c0c1c012b6ed4296fb1cda7a851bb097a $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
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

/**
 * \file
 * This file defines libvlc external API
 */

/**
 * \defgroup libvlc LibVLC
 * LibVLC is the external programming interface of the VLC media player.
 * It is used to embed VLC into other applications or frameworks.
 * @{
 */

#ifndef VLC_LIBVLC_H
#define VLC_LIBVLC_H 1

#if defined (WIN32) && defined (DLL_EXPORT)
# define VLC_PUBLIC_API __declspec(dllexport)
#else
# define VLC_PUBLIC_API
#endif

#ifdef __LIBVLC__
/* Avoid unuseful warnings from libvlc with our deprecated APIs */
#   define VLC_DEPRECATED_API VLC_PUBLIC_API
#elif defined(__GNUC__) && \
      (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
# define VLC_DEPRECATED_API VLC_PUBLIC_API __attribute__((deprecated))
#else
# define VLC_DEPRECATED_API VLC_PUBLIC_API
#endif

# ifdef __cplusplus
extern "C" {
# endif

#include <stdarg.h>
#include <vlc/libvlc_structures.h>

/** \defgroup libvlc_core LibVLC core
 * \ingroup libvlc
 * Before it can do anything useful, LibVLC must be initialized.
 * You can create one (or more) instance(s) of LibVLC in a given process,
 * with libvlc_new() and destroy them with libvlc_release().
 *
 * \version Unless otherwise stated, these functions are available
 * from LibVLC versions numbered 1.1.0 or more.
 * Earlier versions (0.9.x and 1.0.x) are <b>not</b> compatible.
 * @{
 */

/** \defgroup libvlc_error LibVLC error handling
 * @{
 */

/**
 * A human-readable error message for the last LibVLC error in the calling
 * thread. The resulting string is valid until another error occurs (at least
 * until the next LibVLC call).
 *
 * @warning
 * This will be NULL if there was no error.
 */
VLC_PUBLIC_API const char *libvlc_errmsg (void);

/**
 * Clears the LibVLC error status for the current thread. This is optional.
 * By default, the error status is automatically overridden when a new error
 * occurs, and destroyed when the thread exits.
 */
VLC_PUBLIC_API void libvlc_clearerr (void);

/**
 * Sets the LibVLC error status and message for the current thread.
 * Any previous error is overridden.
 * \return a nul terminated string in any case
 */
const char *libvlc_vprinterr (const char *fmt, va_list ap);

/**
 * Sets the LibVLC error status and message for the current thread.
 * Any previous error is overridden.
 * \return a nul terminated string in any case
 */
const char *libvlc_printerr (const char *fmt, ...);

/**@} */

/**
 * Create and initialize a libvlc instance.
 * This functions accept a list of "command line" arguments similar to the
 * main(). These arguments affect the LibVLC instance default configuration.
 *
 * \version
 * Arguments are meant to be passed from the command line to LibVLC, just like
 * VLC media player does. The list of valid arguments depends on the LibVLC
 * version, the operating system and platform, and set of available LibVLC
 * plugins. Invalid or unsupported arguments will cause the function to fail
 * (i.e. return NULL). Also, some arguments may alter the behaviour or
 * otherwise interfere with other LibVLC functions.
 *
 * \warning
 * There is absolutely no warranty or promise of forward, backward and
 * cross-platform compatibility with regards to libvlc_new() arguments.
 * We recommend that you do not use them, other than when debugging.
 *
 * \param argc the number of arguments (should be 0)
 * \param argv list of arguments (should be NULL)
 * \return the libvlc instance or NULL in case of error
 */
VLC_PUBLIC_API libvlc_instance_t *
libvlc_new( int argc , const char *const *argv );


/**
 * \return a static entry point for a module, suitable for passing to
 * libvlc_new_with_builtins. This is to be used when you want to statically
 * link to a module.
 *
 * Note, statically linking to a module will results in nearly zero speed gain
 * and increased memory usage. Use with caution.
 */

#define vlc_plugin(module) & vlc_plugin_entry(module)

#define vlc_plugin_entry(module) vlc_entry__ ## module
#define vlc_declare_plugin(module) extern void *vlc_plugin_entry(module);

/**
 * Create and initialize a libvlc instance.
 *
 * \param argc the number of arguments
 * \param argv command-line-type arguments
 * \param builtins a NULL terminated array of \see vlc_plugin.
 * \return the libvlc instance or NULL in case of error
 * @begincode
 * {
 *     vlc_declare_plugin(mp4);
 *     vlc_declare_plugin(dummy);
 *     const void **builtins = { vlc_plugin(mp4), vlc_plugin(dummy), NULL };
 *     libvlc_instance_t *vlc = libvlc_new_with_builtins(argc, argv, builtins);
 * }
 * @endcode
 */
VLC_PUBLIC_API libvlc_instance_t *
libvlc_new_with_builtins( int argc , const char *const *argv, const void **builtins);


/**
 * Decrement the reference count of a libvlc instance, and destroy it
 * if it reaches zero.
 *
 * \param p_instance the instance to destroy
 */
VLC_PUBLIC_API void libvlc_release( libvlc_instance_t *p_instance );

/**
 * Increments the reference count of a libvlc instance.
 * The initial reference count is 1 after libvlc_new() returns.
 *
 * \param p_instance the instance to reference
 */
VLC_PUBLIC_API void libvlc_retain( libvlc_instance_t *p_instance );

/**
 * Try to start a user interface for the libvlc instance.
 *
 * \param p_instance the instance
 * \param name interface name, or NULL for default
 * \return 0 on success, -1 on error.
 */
VLC_PUBLIC_API
int libvlc_add_intf( libvlc_instance_t *p_instance, const char *name );

/**
 * Registers a callback for the LibVLC exit event. This is mostly useful if
 * you have started at least one interface with libvlc_add_intf().
 * Typically, this function will wake up your application main loop (from
 * another thread).
 *
 * \param p_instance LibVLC instance
 * \param cb callback to invoke when LibVLC wants to exit
 * \param opaque data pointer for the callback
 * \warning This function and libvlc_wait() cannot be used at the same time.
 * Use either or none of them but not both.
 */
VLC_PUBLIC_API
void libvlc_set_exit_handler( libvlc_instance_t *p_instance,
                              void (*cb) (void *), void *opaque );

/**
 * Waits until an interface causes the instance to exit.
 * You should start at least one interface first, using libvlc_add_intf().
 *
 * \param p_instance the instance
 */
VLC_PUBLIC_API
void libvlc_wait( libvlc_instance_t *p_instance );

/**
 * Sets the application name. LibVLC passes this as the user agent string
 * when a protocol requires it.
 *
 * \param p_instance LibVLC instance
 * \param name human-readable application name, e.g. "FooBar player 1.2.3"
 * \param http HTTP User Agent, e.g. "FooBar/1.2.3 Python/2.6.0"
 * \version LibVLC 1.1.1 or later
 */
VLC_PUBLIC_API
void libvlc_set_user_agent( libvlc_instance_t *p_instance,
                            const char *name, const char *http );

/**
 * Retrieve libvlc version.
 *
 * Example: "1.1.0-git The Luggage"
 *
 * \return a string containing the libvlc version
 */
VLC_PUBLIC_API const char * libvlc_get_version(void);

/**
 * Retrieve libvlc compiler version.
 *
 * Example: "gcc version 4.2.3 (Ubuntu 4.2.3-2ubuntu6)"
 *
 * \return a string containing the libvlc compiler version
 */
VLC_PUBLIC_API const char * libvlc_get_compiler(void);

/**
 * Retrieve libvlc changeset.
 *
 * Example: "aa9bce0bc4"
 *
 * \return a string containing the libvlc changeset
 */
VLC_PUBLIC_API const char * libvlc_get_changeset(void);

/**
 * Frees an heap allocation returned by a LibVLC function.
 * If you know you're using the same underlying C run-time as the LibVLC
 * implementation, then you can call ANSI C free() directly instead.
 *
 * \param ptr the pointer
 */
VLC_PUBLIC_API void libvlc_free( void *ptr );

/** \defgroup libvlc_event LibVLC asynchronous events
 * LibVLC emits asynchronous events.
 *
 * Several LibVLC objects (such @ref libvlc_instance_t as
 * @ref libvlc_media_player_t) generate events asynchronously. Each of them
 * provides @ref libvlc_event_manager_t event manager. You can subscribe to
 * events with libvlc_event_attach() and unsubscribe with
 * libvlc_event_detach().
 * @{
 */

/**
 * Event manager that belongs to a libvlc object, and from whom events can
 * be received.
 */
typedef struct libvlc_event_manager_t libvlc_event_manager_t;

struct libvlc_event_t;

/**
 * Type of a LibVLC event.
 */
typedef int libvlc_event_type_t;

/**
 * Callback function notification
 * \param p_event the event triggering the callback
 */
typedef void ( *libvlc_callback_t )( const struct libvlc_event_t *, void * );

/**
 * Register for an event notification.
 *
 * \param p_event_manager the event manager to which you want to attach to.
 *        Generally it is obtained by vlc_my_object_event_manager() where
 *        my_object is the object you want to listen to.
 * \param i_event_type the desired event to which we want to listen
 * \param f_callback the function to call when i_event_type occurs
 * \param user_data user provided data to carry with the event
 * \return 0 on success, ENOMEM on error
 */
VLC_PUBLIC_API int libvlc_event_attach( libvlc_event_manager_t *p_event_manager,
                                        libvlc_event_type_t i_event_type,
                                        libvlc_callback_t f_callback,
                                        void *user_data );

/**
 * Unregister an event notification.
 *
 * \param p_event_manager the event manager
 * \param i_event_type the desired event to which we want to unregister
 * \param f_callback the function to call when i_event_type occurs
 * \param p_user_data user provided data to carry with the event
 */
VLC_PUBLIC_API void libvlc_event_detach( libvlc_event_manager_t *p_event_manager,
                                         libvlc_event_type_t i_event_type,
                                         libvlc_callback_t f_callback,
                                         void *p_user_data );

/**
 * Get an event's type name.
 *
 * \param event_type the desired event
 */
VLC_PUBLIC_API const char * libvlc_event_type_name( libvlc_event_type_t event_type );

/** @} */

/** \defgroup libvlc_log LibVLC logging
 * libvlc_log_* functions provide access to the LibVLC messages log.
 * This is used for debugging or by advanced users.
 * @{
 */

/**
 * Return the VLC messaging verbosity level.
 *
 * \param p_instance libvlc instance
 * \return verbosity level for messages
 */
VLC_PUBLIC_API unsigned libvlc_get_log_verbosity( const libvlc_instance_t *p_instance );

/**
 * Set the VLC messaging verbosity level.
 *
 * \param p_instance libvlc log instance
 * \param level log level
 */
VLC_PUBLIC_API void libvlc_set_log_verbosity( libvlc_instance_t *p_instance, unsigned level );

/**
 * Open a VLC message log instance.
 *
 * \param p_instance libvlc instance
 * \return log message instance or NULL on error
 */
VLC_PUBLIC_API libvlc_log_t *libvlc_log_open( libvlc_instance_t *p_instance );

/**
 * Close a VLC message log instance.
 *
 * \param p_log libvlc log instance or NULL
 */
VLC_PUBLIC_API void libvlc_log_close( libvlc_log_t *p_log );

/**
 * Returns the number of messages in a log instance.
 *
 * \param p_log libvlc log instance or NULL
 * \return number of log messages, 0 if p_log is NULL
 */
VLC_PUBLIC_API unsigned libvlc_log_count( const libvlc_log_t *p_log );

/**
 * Clear a log instance.
 *
 * All messages in the log are removed. The log should be cleared on a
 * regular basis to avoid clogging.
 *
 * \param p_log libvlc log instance or NULL
 */
VLC_PUBLIC_API void libvlc_log_clear( libvlc_log_t *p_log );

/**
 * Allocate and returns a new iterator to messages in log.
 *
 * \param p_log libvlc log instance
 * \return log iterator object or NULL on error
 */
VLC_PUBLIC_API libvlc_log_iterator_t *libvlc_log_get_iterator( const libvlc_log_t *p_log );

/**
 * Release a previoulsy allocated iterator.
 *
 * \param p_iter libvlc log iterator or NULL
 */
VLC_PUBLIC_API void libvlc_log_iterator_free( libvlc_log_iterator_t *p_iter );

/**
 * Return whether log iterator has more messages.
 *
 * \param p_iter libvlc log iterator or NULL
 * \return true if iterator has more message objects, else false
 */
VLC_PUBLIC_API int libvlc_log_iterator_has_next( const libvlc_log_iterator_t *p_iter );

/**
 * Return the next log message.
 *
 * The message contents must not be freed
 *
 * \param p_iter libvlc log iterator or NULL
 * \param p_buffer log buffer
 * \return log message object or NULL if none left
 */
VLC_PUBLIC_API libvlc_log_message_t *libvlc_log_iterator_next( libvlc_log_iterator_t *p_iter,
                                                               libvlc_log_message_t *p_buffer );

/** @} */

/**
 * Description of a module.
 */
typedef struct libvlc_module_description_t
{
    char *psz_name;
    char *psz_shortname;
    char *psz_longname;
    char *psz_help;
    struct libvlc_module_description_t *p_next;
} libvlc_module_description_t;

libvlc_module_description_t *libvlc_module_description_list_get( libvlc_instance_t *p_instance, const char *capability );

/**
 * Release a list of module descriptions.
 *
 * \param p_list the list to be released
 */
VLC_PUBLIC_API
void libvlc_module_description_list_release( libvlc_module_description_t *p_list );

/**
 * Returns a list of audio filters that are available.
 *
 * \param p_instance libvlc instance
 *
 * \return a list of module descriptions. It should be freed with libvlc_module_description_list_release().
 *         In case of an error, NULL is returned.
 *
 * \see libvlc_module_description_t
 * \see libvlc_module_description_list_release
 */
VLC_PUBLIC_API
libvlc_module_description_t *libvlc_audio_filter_list_get( libvlc_instance_t *p_instance );

/**
 * Returns a list of video filters that are available.
 *
 * \param p_instance libvlc instance
 *
 * \return a list of module descriptions. It should be freed with libvlc_module_description_list_release().
 *         In case of an error, NULL is returned.
 *
 * \see libvlc_module_description_t
 * \see libvlc_module_description_list_release
 */
VLC_PUBLIC_API
libvlc_module_description_t *libvlc_video_filter_list_get( libvlc_instance_t *p_instance );

/** @} */
/** @} */

# ifdef __cplusplus
}
# endif

#endif /* <vlc/libvlc.h> */
