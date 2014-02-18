What?
=====
Liblogging is an easy to use library for logging. It offers an enhanced
replacement for the syslog() call, but retains its ease of use.

If you dig deeper, liblogging actually has three components, which address
different needs.

stdlog
------
This is the core component. **Think of it as a the next version of the
syslog(3) API.** We retain the easy semantics, but make it more sophisticated
"behind the scenes" with better support for multiple threads and flexibility
for different log destinations.

Right now, it actually does more or less what syslog() did.  In the next couple
of weeks, this will change. In the future, it will support different log
destinations, like

* syslog()
* systemd journal native API
* unix domain socket [very soon]
* files

The key point here is that we provide a **separation of concerns**: the
application developer will do the logging calls, but the sysadmin will
be able to configure where log messages actually are send to. We will
use a pluggable driver layer to do so.

This follows much the successful log4j paradigm. However, we have a
focus on simplicity. No app developer likes logging, and so we really
want to make it as easy, simple and unintrusive as syslog() was.

An interesting side-effect of that approach is that an application developer
can write code that natively logs to the journal on platforms that support
it but uses different methods on platforms that do not.

journalemu
----------
This component (not yet committed) emulates the most important logging
calls of systemd journal. This permits applications to be written to this
spec, even though the journal is not available on all platforms. Of course,
we recommend writing directly to libstdlog, as this solves the problem
more cleanly.

rfc3195
-------
This is liblogging's original component. Back in 2002, we thought that
logging would be taken over by rfc3195 and thus we begun working on a
library -liblogging- that makes this easy. While the lib is there, rfc3195
has turned out to be a big failure. This component is no longer enhanced,
but it is still available for those apps that need it.

Motivation for this library
===========================
The syslog(3) API is the de-facto standard for application logging on Linux
and Unix. While very simplistic, this is exactly the main feature the app
developer wants. We want to keep this but improve it to

* provide separation of concerns as described above under the stdlog component
* support reentrancy
* provide multiple concurrent log channels to different destinations
* provide an interface for structured logging - but only if there
  is a real demand for it (to be seen based on feedback)

Libloging's goal is also to be a very slim library without any notable memory
or performance footprint (and also simple and small code).

Note to packagers
=================
We recommend to create three different packages for this library, one for
each component. As we do not anticipate new applications to use the
rfc3195 component, we do not suggest building a package for it. On systems
where systemd journal is present, there is hardly a point in packaging
the journalemu component. So *in esscence, the stdlog component is the only
one we suggest to be packaged on many platforms*.

Depending on distro policies, package names like *liblogging1-stdlog* are 
suggested. This prevents confusion with liblogging v0, which only supported
rfc3195.

History
=======
Liblogging is around since 2002. See [HISTORY](HISTORY.md) file for some
background information on how it evolved and why it is structured like
it currently is.
