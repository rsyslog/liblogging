=================
liblogging-stdlog
=================

------------------------
standard logging library
------------------------

:Author: Rainer Gerhards <rgerhards@adiscon.com>
:Date: 2014-02-22
:Manual section: 3
:Manual group: standard logging library

SYNOPSIS
========

::
   
   #include <liblogging/stdlog.h>

   const char* stdlog_version(void);

   int stdlog_init(int options);
   void stdlog_deinit();

   stdlog_channel_t stdlog_open(const char *ident,
             const int options, const int facility,
             const char *channelspec);
   int stdlog_log(stdlog_channel_t channel, const int severity,
                  const char *fmt, ...);
   int stdlog_log_b(stdlog_channel_t channel, const int severity,
                  char *buf, const size_t lenbuf,
                  const char *fmt, ...);
   int stdlog_vlog(stdlog_channel_t channel, const int severity,
                  const char *fmt, va_list ap);
   int stdlog_vlog_b(stdlog_channel_t channel, const int severity,
                  char *buf, const size_t lenbuf,
                  const char *fmt, va_list ap);
   void stdlog_close(stdlog_channel_t channel);

   size_t stdlog_get_msgbuf_size(void);
   const char *stdlog_get_dflt_chanspec(void);

DESCRIPTION
===========

**stdlog_version()** returns the version string for the library
currently being used (e.g. "1.0.2"). It may be called at any time.
If a specific (minimal) version of the library is required, it is
suggested to do runtime checks via **stdlog_version()** before
**stdlog_init()** is called.

**stdlog_init()** is used to initialize the logging system.
It must only be called once during the lifetime of a process. If no
special options are desired, stdlog_init() is optional. If it is not
called, the first call to any of the other calls will initiate it.
This feature is primarily for backward compatibility with how the
legacy **syslog(3)** API worked. It does not play well with multi-threaded
applications. With them, call stdlog_init() explicitly from the
startup thread. The parameter *options* contains one or more of
the library options specified in their own section below.

**stdlog_deinit(void)** is used to clean up resources including closing
file handles at library exit. No library calls are permitted after it
has been called. It's usage is optional if no cleanup is required (this
will leave resource leaks which will be reported by tools like
valgrind).


**stdlog_open()** is used to open a log channel which can be used in 
consecutive calls to *stdlog_log()*. The string given to *ident* is
used to identify the message source. It's handling is depending on the
output driver. For example, the file: and syslog: drivers prepend it 
to the message, while the journal: driver ignores it (as the journal
automatically identifies messages based on the application who submits
them. In general, you can think of it as being equivalent to the
*ident* specified in the traditional **openlog(3)** call. The value
given in *options* controls handling of the channel. It can be used to
override options set during **stdlog_init()**. Note that for signal-safeness
you need to specify **STDLOG_SIGSAFE**. The *facility* field contains a
facility similar to the traditional syslog facility. Again, it is 
driver-dependent on how this field is actually used. The *channelspec*
filed is a **channel specification** string, which allows to control
the destination of this channel. To use the default output channel
specification, provide NULL to *channelspec*. Doing so is highly suggested
if there is no pressing need to do otherwise.

**stdlog_close()** is used to close the associated channel. The channel
specifier must not be used after *stdlog_close()* has been called. If done
so, unpredictable behavior will happen, as the memory it points to has
been free'ed.

**stdlog_log()** is the equivalent to the **syslog(3)** call. It offers a
similar interface, but there are notable differences. The *channel* 
parameter is used to specify the log channel to use to. Use *NULL* to select
the default channel. This is sufficient for most applications. The *severity*
field contains a syslog-like severity.  The remaining arguments are a format,
as in **printf(3)** and any arguments required by the format. Note that some
restrictions apply to the format in signal-safe mode (described below).
The **stdlog_log()** supports log message sizes
of slightly less than 4KiB. The exact size depends on the log driver
and parameters specified in **stdlog_open()**. The reason is that the
log drivers may need to add headers and trailers to the message
text, and this is done inside the same 4KiB buffer that is also used for
the actual message text. For example, the "syslog:" driver adds a traditional
syslog header, which among others contains the *ident* string provided
by **stdlog_open()**. If the complete log message does not fit into
the buffer, it is silently truncated. The formatting buffer is allocated
on the stack.

Note that the 4Kib buffer size is a build time default. As such,
distributions may change it. To obtain the size limit that the
linked in instance of libloggin-stdlog was build with, use
**stdlog_get_msgbuf_size()**.
You may also use the **stdlogctl(1)** utility to find out the build
time settings for the installed version of liblogging-stdlog.

**stdlog_log_b()** is almost equivalent to **stdlog_log()**, except that
the caller can provide a formatting work buffer. This is done via the *buf*
and *buflen* parameters. This permits to use both smaller and larger buffer
sizes. For embedded systems (or signal handlers), this may be convenient to
reduce the amount of stack space required. Also, it is useful if very large
messages are to be logged. Note that while there is no upper limit on the
buffer size per se, the log drivers may have some limits. In general, up
to 64KiB of buffer should work with all drivers.

The **stdlog_vlog()** and **stdlog_vlog_b()** calls are equivalent to
**stdlog_log()** and **stdlog_log_b()** except that they take a *va_list*
argument.

Use **stdlog_get_dflt_chanspec()** to obtain the default channel specification.
This must be called only after **stdlog_init()** has been called.

OPTIONS
=======
Options modify library behavior. They can be specified in **stdlog_init()**
and **stdlog_open()** calls. The **stdlog_init()** call is used to set
default options. These are applied if channels are automatically created or
the *STDLOG_USE_DFLT_OPTS* option is used in **stdlog_open()**. Otherwise,
options provided to **stdlog_open()** are not affected by the global option
set.

The following options can be given:

:STDLOG_USE_DFLT_OPTS: is used to indicate that the **stdlog_open()** call
   shall use the default global options. If this option is given, on other
   options can be set. Trying to do so results in an error. Note that this
   option is *not* valid to for the **stdlog_init()** call.

:STDLOG_SIGSAFE: request signal-safe mode. If and only if this is 
   specified library calls are signal-safe. Some restrictions apply
   in signal-safe mode. See description below for details.

FACILITIES
==========
The following facilities are supported. Please note that they are mimicked
after the traditional syslog facilities, but liblogging-stdlog uses
different numerical values. This is necessary to provide future enhancements.
Do **not** use the LOG_xxx #defines from syslog.h but the following
STDLOG_xxx defines:

::

   STDLOG_KERN     - kernel messages
   STDLOG_USER	   - random user-level messages
   STDLOG_MAIL	   - mail system
   STDLOG_DAEMON   - system daemons
   STDLOG_AUTH	   - security/authorization messages
   STDLOG_SYSLOG   - messages generated internally by syslogd
   STDLOG_LPR	   - line printer subsystem
   STDLOG_NEWS	   - network news subsystem
   STDLOG_UUCP	   - UUCP subsystem
   STDLOG_CRON     - clock daemon
   STDLOG_AUTHPRIV - security/authorization messages (private)
   STDLOG_FTP      - ftp daemon

   STDLOG_LOCAL0   - reserved for application use
   STDLOG_LOCAL1   - reserved for application use
   STDLOG_LOCAL2   - reserved for application use
   STDLOG_LOCAL3   - reserved for application use
   STDLOG_LOCAL4   - reserved for application use
   STDLOG_LOCAL5   - reserved for application use
   STDLOG_LOCAL6   - reserved for application use
   STDLOG_LOCAL7   - reserved for application use

Regular applications should use facilities in the **STDLOG_LOCALx**
range. Non-privileged applications may not be able to use
all of the system-defined facilities. Note that it is also safe to
refer to application specific facilities via

::

   STDLOG_LOCAL0 + offset

if offset is in the range of 0 to 7.

SEVERITY
========
The following severities are supported:

::

   STDLOG_EMERG	  - system is unusable
   STDLOG_ALERT   - action must be taken immediately
   STDLOG_CRIT    - critical conditions
   STDLOG_ERR     - error conditions
   STDLOG_WARNING - warning conditions
   STDLOG_NOTICE  - normal but significant condition
   STDLOG_INFO    - informational
   STDLOG_DEBUG   - debug-level messages

These reflect the traditional syslog severity mappings. Note that
different output drivers may have different needs and may map
severities into a smaller set.

THREAD- AND SIGNAL-SAFENESS
===========================

These calls are thread- and signal-safe:

* **stdlog_version()**
* **stdlog_get_msgbuf_size()**
* **stdlog_get_dflt_chanspec()**

These calls are **not** thread- or signal-safe:

* **stdlog_init()**
* **stdlog_deinit()**
* **stdlog_open()**
* **stdlog_close()**

For **stdlog_log()**, **stdlog_vlog()**, **stdlog_log_b()**, and
**stdlog_vlog_b()**, it depends:

* if the channel has been opened with the *STDLOG_SIGSAFE* option,
  the call is both thread-safe and signal-safe.
* if the library has been initialized by **stdlog_init()** or the channel has
  been opened by **stdlog_open()**, the call is thread-safe but **not**
  signal-safe.
* if the library has not been initialized and the default (NULL) channel is
  used, the call is neither thread- nor signal-safe.

For **stdlog_log_b()** and **stdlog_vlog_b()** the caller must also ensure
that the provided formatting
buffer supports the desired thread- and signal-safeness. For example, if a
static buffer is used, thread-safeness is not given. For signal-safeness,
typically a buffer allocated on the signal handler's stack is needed.

For multi-threaded applications, it is **highly recommended** to initialize
the library via **stdlog_init()** on the main thread **before** any other
threads are started.

Thread- and signal-safeness, if given, does not require different
channels. It is perfectly fine to use the same channel in multiple threads.
Note however that interrupted system calls will not
be retried. An error will be returned instead. This may happen if a thread
is inside a **stdlog_log()** call while an async signal handler using that
same call is activated. Depending on timing, the first call may or may not
complete successfully. It is the caller's chore to check return status and
do retries if necessary.

Finally, thread- and signal-safeness depend on the log driver. At the time
of this writing,
the "syslog:" and "file:" drivers are thread- and signal-safe while the
current "journal:" driver is thread- but not signal-safe. To the best of
our knowledge, the systemd team is working on making the API we depend on
signal-safe. If this is done, the driver itself is also signal-safe (the
restriction results from the journal API).

RESRICTIONS IN SIGNAL-SAFE MODE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When signal-safeness is requested, the set of supported printf formats
is restricted. This is due to the fact that the standard printf routines
cannot be called and so a smaller signal-safe printf implementation that is
part of *liblogging-stdlog* is used instead.

It has the following restrictions:

* flag characters are not supported
* field width and precision fields are accepted but silently ignored
* the following length modifiers are supported: **l, ll, h, hh, z**
* the following conversion specifiers are supported: **s, i, d, u, x, X,
  p, c, f** (where **f** is always formatted as "%.2f")
* only the following control character escapes are supported:
  **\\n, \\r, \\t, \\\\**.
  Please note that it is **not** advisable to include control characters
  in log records. Log drivers and log back-end systems may remove them.

CHANNEL SPECIFICATIONS
======================
The channel is described via a single-line string. Currently, the following
channels can be selected:

* "syslog:", which is the traditional syslog output to /dev/log
* "uxsock:<name>", which writes messages to the local unix socket
  *name*. The message is formatted in traditional syslog-format.
* "journal:", which emits messages via the native systemd journal API
* "file:<name>", which writes messages in a syslog-like format to
  the file specified as *name*

If no channel specification is given, the default is "syslog:". The
default channel can be set via the **LIBLOGGING_STDLOG_DFLT_LOG_CHANNEL**
environment variable.

Not all output channel drivers are available on all platforms. For example,
the "journal:" driver is not available on BSD. It is highly suggested that
application developers **never** hard-code any channel specifiers inside
their code but rather permit the administrator to configure these. If there
is no pressing need to select different channel drivers, it is suggested
to rely on the default channel spec, which always can be set by the
system administrator.

RETURN VALUE
============

When successful **stdlog_init()** and **stdlog_log()** return zero and 
something else otherwise. **stdlog_open()** returns a channel descriptor
on success and *NULL* otherwise. In case of failure *errno* is set
appropriately.

Note that the traditional **syslog(3)** API does not return any success
state, so any failures are silently ignored. In most cases, this works
sufficiently reliably. If this level of reliability is sufficient, the
return code of **stdlog_log()** does not need to be checked. This is
probably the case for most applications.

If finding out about the success
of the logging operation is vital to the application, the return code
can be checked. Note that you must not try to find out the exact failure
cause. If the return is non-zero, something in the log system did not work
correctly. It is suggested that the logging operation is re-tried in this
case, and if it fails again it is suggested that the channel is closed and
re-opened and then the operation re-tried. During failures, partial records
may be logged. This is the same what could happen with **syslog(3)**. Again,
in general it should not be necessary to check the return code of
**stdlog_log()**.

The **stdlog_deinit()** and **stdlog_close()** calls do not return
any status.


EXAMPLES
========

A typical single-threaded application just needs to know about
the **stdlog_log()** call:

::

    stdlog_log(NULL, STDLOG_NOTICE, "New session %d of user %s",
               sessid, username);

Being thread- and signal-safe requires a little bit more of setup:

::

    /* on main thread */
    status = stdlog_init(STDLOG_SIGSAFE);

    /* here comes the rest of the code, including worker
     * thread startup.
     */


    /* And do this in threads, signal handlers, etc: */
    stdlog_log(NULL, STDLOG_NOTICE, "New session %d of user %s",
               sessid, username);

If you need just a small formatting buffer (or a large one), you can
provide the memory yourself:

::

    char buf[512];
    stdlog_log_b(NULL, STDLOG_NOTICE, buf, sizeof(buf),
                 "New session %d of user %s", sessid, username);


SEE ALSO
========
**stdlogctl(1)**, **syslog(3)**

COPYRIGHT
=========

This page is part of the *liblogging* project, and is available under
the same BSD 2-clause license as the rest of the project.
