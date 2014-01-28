Liblogging was created in 2002 when there was a lot of enthusiasm
about rfc3195 (and the underlying BEEP protocol). It's core mission
was to provide an easy way to do logging, which at that time we
assumed to mean "speak rfc3195". Originally, it was developed as a
closed source solution on Windows, but Adiscon relatively soon
agreed to make it available as an open source library. This was
done in the hope that the library would help spread rfc3195.

Note that the library's design goal, from the very beginning, was
to be as unintrusive as possible. RFC3195 offers multiplexing
capabilites and multiple channels and the few other existing 
library options required dedicated threading models or even
took over the main processing loop from the application.
Liblogging in contrast focussed on an as simple as possible
(for rfc3195 ;)) API and was able to be used with any threading
model.

In the early days, liblogging was still primarily developed on
Windows until Devin Kowatch helped make it compile under *nix in
2003.

In the coming years, liblogging kept a prime library for logging,
albeit mostly in the rfc3195 context. (Not so) unfortuantely,
rfc3195 never took off in the logging world. It had some bright
phases but now (in 2014) I think it's fair to say it was a failure.

Liblogging was maintained and kept stable over many years. In 2013
we decided that it should be upgraded to support non-rfc3195 logging,
living really up to it's name of "liblogging". In spring 2013 we
looked at emulating systemd journal calls, enabling apps that use
it to be able to run on non-systemd systems. While we prototype things,
we did not dig much deeper into this as by that time (and even today)
there is not much demand for the journal API in any case.

More important is simple but feature-rich logging. As such, we added
a third component in 2014, libstdlog. It is meant to provide basic
logging services to C programs, but make it possible to use different
sinks without the need to rebuild applications. In a sense, it's like
a very lightweight log4j -- without all the formatting stuff and so.

We believe that liblogging will play in important role when it comes
to make applications independent from the logging provider. This is
more important with the journal than before, because the journal
structured logging API (while good) is not universally available.

Future will tell how we reach this goal. In any case, you now know
why there are three components inside liblogging, and you also know
that liblogging's missing is the same as when it was initially designed,
even though the technology to achive these goals has greatly changed.
