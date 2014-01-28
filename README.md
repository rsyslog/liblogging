What?
=====
Liblogging is an easy to use library for logging. It offers an enhanced
replacement for the syslog() call, but retains its ease of use.

If you dig deeper, liblogging actually has three components, which address
different needs.

stdlog
---------
This is the component of interest for most users. It offers the syslog()
API replacement. Right now, it actually does more or less what syslog() did.
In the future, however, it will support different log destinations, like

* syslog()
* systemd journal native API
* unix domain socket
* files

This will permit an application developer to use a single, very easy to
use set of function calls to talk to any logging system. The admin will
be able to configure which destination to actually use. This is especially
important with the introduction of systemd journal, which takes over the
regular syslog() socket. While this is fine for usual programs, it creates
a problem for applications that produce huge amounts of log data and need
to do so fast. With libstdlog, logging can go either to the journal or
some other source without the need to modify an application or rebuild it.

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

History
=======
Liblogging is around since 2002. See [HISTORY](HISTORY.md) file for some
background information on how it evolved and why it is structured like
it currently is.
