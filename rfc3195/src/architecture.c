/*! \mainpage liblogging - a slim library for reliable syslog 
 *
 * \section intro Introduction
 *
 * RFC 3195 offers reliable connections between syslog clients
 * and collectors (servers, daemons). This is highly desirable
 * in todays networking world. However, RFC 3195 was initially
 * not implemented because many implementors thought it was too
 * complex. The reason for this is that 3195 is a BEEP profile
 * (see RFC 3080 and 3081). BEEP itself is a very powerful, yet
 * relatively complex protocol. Full BEEP libraries are hard to
 * find and often place restrictions on the developer, especially
 * in regard to threading models and such.
 *
 * Liblogging takes a different approach. It is a library specifcally
 * created to talk syslog, NOT general BEEP. It provides just
 * the bare functionality needed for syslog, nothing else. 
 * Consequently it is very slim. The authors hope that this will
 * enable developers of even low-powered devices to adopt 
 * the library and provide reliable syslog logging.
 *
 * This library has been named liblogging because it may
 * eventually contain code for additional logging methods
 * in the future (like "plain" UDP syslog, 3195 COOKED and
 * syslog-sign). But this is no promise ;)
 *
 * If you are interested in "just" using the library, please
 * go straigt to \ref srAPI.h. 
 *
 * Continue to read only if you would like to learn the inner
 * workings or port the lib to a different environment.
 *
 * This library compiles under a variety of platforms. It is
 * originally developed and maintened under Win32. It is frequently
 * ported and tested on Red Hat Linux. These are the platforms (in this
 * order) we have the most confidience in the library on. Others may
 * (though unlikely) show unexpected behaviour.
 *
 * This documentation file here is NOT up to date. However, it will
 * provide you a good overview of the overall design and many specifics.
 * To get the totally right details, please see the respective modules'
 * documentation. Please note that the SEQ handling described below
 * for the listener part is NOT YET implemented - but it is still on
 * the list. As such, the server API should be considered unstable.
 * If you base a server on liblogging, please be prepared to update your
 * server to make it work with future versions of liblogging. Of course
 * we try to keep the impact as low as possible, but that's not 
 * a guarantee!
 *
 * The client side API is stable and changes should not be necessary
 * for future releases (except, of course, you would like to use
 * new features).
 *
 * \section designGoals Design Goals
 * The overall mission of liblogging is to provide a small, easy to
 * use library that will work as an enabler for reliable syslog.
 *
 * There are a number of design goals
 * - self-contained\n
 *   no additional libraries shall be needed
 * - low ressource consumption\n
 *   memory and CPU usage should be kept to a minimum
 *   so that the lib can also be used on low-powered
 *   devices
 * - thread-model agnostic\n
 *   no specific threading model is needed for this
 *   library. Everything can be done from a single
 *   thread, BUT the library is thread-safe so
 *   that it can be called from multiple threads
 *   concurrently.
 * - extensible\n
 *   Even though the initial design includes a very
 *   limited feature set, liblogging should be able to
 *   be extended easily.
 * - portability
 *   That's a hard one. liblogging is developed under
 *   Win32. While we take reasonable care to make
 *   it portable, we only know this once ports have
 *   actually been done.
 *
 * \section architecture Architecture
 *
 * \subsection beeplevel Level of BEEP Compliance
 * In order to achieve our primary goals, we do NOT
 * provide a full BEEP stack. Effectively, you could
 * even argue that we do not provide a BEEP stack
 * but a "BEEP pretender". It implements just what
 * it needs and uses e.g. canned greetings instead
 * of actual ones. It often does only accept some
 * very specific answers and will probably answer
 * with a BEEP ERR if others are provided.
 *
 * All in all, RFC3195 provides a very limited need
 * for BEEP features and we only provide this set.
 * So do NOT expect a small, full BEEP lib - you
 * will be severely disappointed. If you are 
 * interested in basic syslog functionality, this
 * lib is for you. If you want more, you should
 * continue searching. liblogging will probably
 * provide more headache as it is worth in this
 * case.
 *
 * BEEP peers can either be initiators or listeners.
 * Initiators are what traditionally is called
 * "clients" - they talk to a server in order
 * to acomplish some work. "Listeners" are traditionally
 * called servers.
 *
 * This library has different approaches for initiators
 * and listeners. Keep in mind that the lib shall not impose
 * any specific threading model onto its host. As such, we
 * must be able to run on a single thread, even if there
 * are potentially multiple activities going on (like
 * channel 0 and channel 1 operations).
 *
 * \subsubsection initiators Initiators
 * Initiators create a BEEP session to the remote peer.
 * The session object also provides channel 0 management
 * (this is part of the session).
 *
 * Upon creating a session, the session object sends this peers
 * greeting and reads (and stores) the remote peers greeting. 
 * Other than that, nothing happens. This peers greeting will
 * always be empty as we do not advertise any listener 
 * functionality at this stage.
 *
 * Finally, the initiator profile calls the channel 
 * setup function. It provides a pointer to its profile object
 * to the session object. The session object in turn takes the URI 
 * from the profile object and initiates a channel with the remote
 * peer. The peers response is read and interpreted. If it is positive,
 * a new channel object is created and passed onto the initiator profile.
 *
 * One the channel has been created, it is up to the initiator profile
 * to control data exchange. To do so, the channel object provides
 * send and receive methods. As of this release, the send and 
 * receive full messages only and DO NOT provide support for
 * fragmentation.
 *
 * It is important to keep in mind that even though a "simple"
 * syslog raw application will not run more than one channel, there
 * are potentially messages being exchanged to do NOT belong to
 * the actual syslog data exchanged. Most importantly, these are
 * the RFC 3081 SEQ frames, which must be paid attention to. If
 * they are not honered, the session will stall as soon as more 
 * than a few bytes of data are being sent. Other than that,
 * there may be some control messages on channel 0.
 *
 * In order to support these asynchronous channels, we need
 * to provide for an intelligent TCP stream reader. We have
 * decided to implement it in a way that will also facilitate
 * later additions of true multi-channel BEEP sessions.
 *
 * The corenerstone is a session management method called
 * readFrame(). It receives a pointer to the channel object.
 * Its duty, from the channel object point of view, is to
 * obtain the next frame from the physical stream. From the
 * BEEP sesseion point of view, its duty is to read & buffer
 * all incoming data and also do session management and
 * SEQ frame processing. This is done as follows:
 *
 * WHEN RECEIVING DATA, the initiator profile calls
 * the message object and asks it to receive a message.
 * The message object then calls the frame object and
 * asks it to receive the frame. Please note that in
 * this version of the library, fragmentation is not supported
 * and as such there is a one-to-one relationship between
 * messages and frames. Should that be changed in later 
 * versions of the library, the message object can easily
 * be upgraded to receive multiple frames up until the
 * final frame has been received. Back to the frame
 * object: once the frame object's receive method is
 * called, itself calls back into the session object
 * and instructs the session object to gather the data.
 * This is very important, because now the session object
 * is capable of handling intermixed control
 * messages as well as SEQa. This provides a kind of
 * multi-tasking without real multi-threading.
 * When the session object receives control, it first
 * queries the socket (via select) if any data is ready
 * to be received. If so, the session object receives all
 * data (in method DoReceive() until no more data is ready to be received. To
 * actually receive the data, it calls back into another
 * method of the frame object (let's call this method
 * ActualRecvFram() for now). We will describe this method
 * in a short while. Each frame returned from ActualRecvFram()
 * will be examined by the session object. If it is a SEQ
 * frame, the associated channel will be called to process
 * the SEQ immediately. If it is a data frame, it is buffered
 * in an in-memory queue. The queue is on a per-channel basis.
 * Once all messages are processed, the session object looks
 * at the requesting channels data queue. If there is at least
 * one entry, that entry is passed back to the channel object.
 * if there is still no entry, the session object re-itereates
 * its processing, starting with the polling of the incoming
 * data. In order to avoid a tight loop, the session object
 * must Sleep() some milliseconds (at least 10) if there
 * are no frames ready in consequtive calls. Using this
 * mode of operation, the session objects receive frame
 * method will not only provide the next data frame to its
 * uppper layer but will also be able to act on SEQ frames
 * as well as other channel's data. It is also possible that
 * a specifically configured channel/profile may ask the session
 * object to call a callback as soon as data is received.
 * this mode will probably be used in listeneres and provide
 * full multiprocessing. It is not thought out yet.
 *
 * Back the the ActualRecvFram() method of the frame object:
 * it will first receive the 3-byte header and then use the
 * processing that is required by the header type. E.g.
 * SEQ has a very different processing. Once this is done,
 * it will read the channel number and once the channel is known,
 * ActualRecvFrame() will call back into the session object to
 * obtain a handle to this channel's object. This handle
 * will then be used to do seqno verification. The channel
 * object will be updated by ActualRecvFram().
 * 
 * WHEN SENDING DATA, The initiator profile constructs a message 
 * and asks the message object to send it. The message object then
 * constructs the necessary frames (in this release only one!)
 * and then asks the frame object to send this frame. The frame
 * object hands the request down to a send method in the session
 * object. The session object then first checks if there is unread data in
 * the TCP stream. If so, it reads this data off the session. This process
 * is the same as outlined in "when receiving data". In fact, the same
 * method (DoReceive()) is to be called. It takes care of the necessary
 * housekeeping and also processes all SEQ frame not processed so far. 
 * This is a very important step, as an on-the-wire SEQ frame may be
 * necessary to be read before the actual to-be-sent frame can be send.
 * Once DoReceive() returns, the session object sends the frame
 * with the help of the channel object. The channel object must
 * check if the current frame can be fully send (in respect to the
 * current Window size). If it can't, it must flag an error condition
 * and NOT send the frame. It should then return immediately to the
 * session object. If it can send the frame, it must do so and then
 * return with an OK return status to the session object. The session
 * object must check the return value. If the return value indicates
 * the current frame could not be sent due to the Window being 
 * insufficient, the session object must again do a DoReceive().
 * However, this time it can be done in a blocking manner as no
 * further processing is possible without receiving the obviously
 * outstanding SEQ. Once this is done, the session object re-
 * iterates its processing and will be able to send the frame
 * during either this stage or a later one.
 *
 * If the channel object returned with either another error
 * or an OK condition, the session object can also terminate and
 * pass the return status to its upper layer (and so forth until
 * the profile object is reached).
 *
 * \subsubsection listeners Listeners
 * Listeners provide server functionality. They are implemented
 * via the sbLstn Object. Please note that some of the description
 * below may refer to this as part of the session object. It is
 * NO LONGER, this was changed. The writing below will be updated
 * once the implementation is fully done (if not forgotten ;)).
 *
 * Not every potential liblogging user will need to implement
 * listeners. In fact, we assume most will use it for implemnting 
 * clients, only. In order to keep the library footprint small, all
 * session management methods needed by the listener code only
 * has been moved to \ref beepsessionLstn.c beepsessionLstn.c. This
 * way, it will be a separate object module and only be linked in
 * to those applications actually needing it. The prototypes,
 * however, are in beepsession.h as they logically belong there.
 *
 * If somebody for some reason would like to disable the listener
 * functionality at all, simply remove the beepsessionLstn.c file
 * from the build. Be sure to do the same for all listner profiles
 * (and any other profiles you don't need). YOU CAN NOT USE
 * LISTENER PROFILES IF YOU HAVE REMOVED beepsessionLstn.c
 * (for obvious reasons). Trying so will result in linker errors.
 *
 * Channel 0 protocol is not handled via profiles. It is done
 * in BEEPSession handler, only.
 *
 * \paragraph listener-overview Overview
 * Listeners are quite challenge for liblogging's goal to be
 * independent of any threading model. This release will be build
 * around select(), that is it will be running on a single thread
 * for all of its network receive needs. If that model is too simplistic
 * for a caller, the caller may go ahead and use the provided events
 * to call back into some of his multiple threads, doing whatever
 * threading logic he needs to do. I know this is a bit tricky,
 * but it is the current state of affairs. Later versions of the
 * library may provide an alternate mean of threading in which
 * the caller is assumed to create a new thread for each incoming
 * session (not channel). If this happens, of course the currently
 * existing mode will also remain. So don't panic, no code to 
 * change in that event ;-).
 *
 * Anyhow, keep in mind that the select() method is very powerful
 * and - together with proper other processing - can be used to
 * build high-performance servers.
 *
 * While not enforcing this model, we recommend that profiles
 * using this library (and apps using the provided profiles)
 * just pass the data off to some queue (whereever it may be) and
 * process that data asynchronously. Methods have been build
 * into liblogging to do this very efficiently.
 *
 * OK, having said this, the current approach is:
 *
 * At the core of the listener functionality is a method called
 * sbLstnRun() (or a similar name). That method *is* the
 * actual listener. In concept, it just does the following:
 * <ol>
 * <li> It assembles a list of all active sessions (NOT channels!) 
 *    AND the session-initiation listener socket.
 * <li> It calls select() on all of them, asking select() to
 *    return only when new data is available (or a new connect
 *    came in). Please note that this is in fact a bit more
 *    complicated (see the handling of SEQ below), but we keep
 *    it simple here to convey the idea.
 * <li> When select returns, sbLstnRun() checks if it
 *    is a new connection request or data received.
 *    - If it is a new connection request, it is passed on to
 *      the session object to create a new session including
 *      channel 0.
 *    - If it is new data, that data is passed onto the session
 *      object. It will validate it and act on channel 0 management
 *      messages. If it is profile data, it passes the data onto the
 *      registered event hook of the profile in question.
 * </ol>
 * These three steps are repeated until an terminate processing
 * request is issued to sbLstnRun(). This can be done by
 * using a large timeout (10 seconds?) on the select() wait. If the
 * timeout expires and there was no network activity, a simple
 * variable is checked. If it is set to e.g. 0, the loop will
 * be terminated, otherwise it will be continued. The variable
 * may be best called "RunIndicator" or such.
 *
 * If we look at this model, a profile would use the following
 * steps to set up a server:
 * <ul>
 * <li> Create the profile object and populate the required
 *      profile event pointers.
 * <li> Add the profile to the pool of the BEEP listners supported
 *      profiles.
 * </ul>
 * These two steps should be done for each profile supported. When
 * finished with this
 * <ul>
 * <li> The application must call sbLstnRun()
 * </ul>
 * From now on, the server is active and will call back into the
 * profiles as requested. The server will automatically
 * - accept new incoming connections
 * - provide a greeting message specifying the actual listener
 *   profiles
 * - do channel 0 management
 * - call the respective profile hooks for incoming data and
 *   other events (see below)
 *
 * The sbLstnRun() can only be stopped by calling 
 * sbSessTerminteListen(). This, in turn, would be most probably
 * called in response to an external event.
 * This could be either a signal (like SIGUSR1 or SIGINT) or the
 * result of a profile action. It could also be set to auto-terminate
 * after the last open session has been terminated and there are no
 * other active sessions (though this would only be useful for the 
 * most simplistic listenere). Most apropriately, the sbSessTerminteListen()
 * method should be called from a different thread inside the server process.
 * Anyhow, we must not have multiple threads running to run multiple channels
 * concurrently. All can be done on a single thread. The
 * \ref sample-server.c sample-server.c example program is such a simple,
 * single-threaded server. It terminates as soon as there are no more open
 * sessions (OR SHOULD IT TERMINATE ON SIGINT???? - let's see what ports
 * easier...).
 *
 * Please note that sbLstnRun() will construct the greeting message
 * dynamically on the list of profiles available AT THE TIME OF THE CONNECT.
 * In a multi-threading caller, profiles may be added and removed while
 * sbLstnRun() is active. This is a valid action. (\todo The 
 * linked list of profiles is thread-safe). When a profile is added,
 * it is immediatly available for the next connection. If a profile
 * is removed, (\todo implement this later!) the profile is immediately
 * no longer be advertised in the greeting BUT it is actually removed
 * only after the last channel using it has been closed.
 *
 * It is valid, though useless, to call sbLstnRun() whithout any
 * profile being set. This is to support multi-threaed application which
 * set profiles later. Of course, an empty profile list means no channel
 * can be created at all (but the session will be accepted and channel 0
 * be established even in this case).
 *
 * In order to perform its work, the session object keeps a linked list
 * of all active sessions. This is used to feed select(). The current listen
 * channel is managed by sbLstnRun() itself and NOT part of the
 * linked list.
 *
 * \paragraph listenersending Sending Data
 * Well, believe it or not, I forgot that a server must not only receive
 * but send data ;) So this paragraph here is a little disconnected from
 * the rest of the sbLstnRun() description.
 *
 * In order to work successfully with select(), you must make sure that
 * your application never blocks when using sockets. This obviously
 * includes responses send. You can (unfortunately ;)) not assume
 * that a send call will always be non-blocking - even with small
 * amounts of data, transmit queues can be build. As such, we need
 * to find a model which enables us to asynchronously send data that
 * must be sent as a response.
 *
 * Fortunately, this is relatively easy to acomplish. The key is that all
 * IO (including sending) is handled by the listener process itself (the one
 * that controls the select()). As such, we enhance the session object to
 * contain a queue of to-be send data as well as a state variable. The
 * session (of a listener) object is now a state machine. Data
 * is send depending on the current state. Whenever data is to be sent,
 * it is first checked if any send operation is in progress. If not, 
 * it is tried to send the data. If that succeeds, no further action
 * is taken. If it does not succeed, however, the to be send data
 * is enqueued on the session object.
 *
 * Method sbLstnRun() checks if sessions have outstanding send requests.
 * If so, it includes their sockets in the select() call. If select
 * returns with these sockets, it is tried to send as many data as
 * possible. Data successfully send is removed from the queue. Sending
 * stops if either the queue is empty or the send would block. In the
 * later case, the rest of the queue will be tried to send after the
 * next select call (and so on).
 * 
 * A frame can be sent if
 * - the socket is ready for sending
 * - the remaining window is large enough
 *
 * If either of the two is not true, sending needs to be deferred.
 * If a frame is already partly send, no other frame on the same session
 * can be send before the currently "active" frame has completely been
 * sent. 


 * When receiving data:
 * The frame needs to be constructed on the fly. The IO handler, when
 * receivin, has two different states:
 * - in header
 * - in body
 * 
 * While in header, it is looking for the terminating CRLF. There should
 * also a saveguard (e.g. 4K) so that it can not be overrun. While it is
 * state = in_header, no processing takes place. Each received byte is
 * added to the received header line. As soon as CRLF are received,
 * the header line is parsed and a protocol error is then detected (as such,
 * no errors are caught). 
 * When in body, messages are received until the specified message limit.
 * Then, the status changes to "in trailer" which then will verify new
 * incoming messages against the trailer.
 *
 * \paragraph threadingmodel Threading Model
 * As can be seen, no specific threading model is needed, we can also operate
 * a multi-tasking server on a single thread. HOWEVER, we recommend the
 * following scheme to users of this library:
 *
 * - Have at least on thread for managing your application. That thread should
 *   call in to add and/or remove profiles to the listener.
 * - Have one other thread whoms sole purpose is to call sbLstnRun()
 *   and provide a worker for that as well as the profile callbacks.
 * - Have at least one other thread that will actually process the data
 *   received by the profiles.
 *
 * It is recommended that the profiles (on the sbLstnRun() thread) just
 * do profile handling and put the received data into some queue that is then
 * processed by the third thread pool. They also need to keep track of 
 * acknowledgments and thus the queue size. They should not carry out any
 * other action and return as quickly as possible to sbLstnRun().
 * THIS IS A CRITICAL REQUIREMENT - if the profile callbacks do too lengthy
 * operations (or even waits), they can thrash the whole multithreaded
 * application.
 *
 * Of course, if the applications is single-threaded, the profile callbacks
 * can do (and must do) whatever processing is needed, no matter how long
 * it will take. ;-)
 *
 * We strongly encourage implementors to use the above model with at least
 * 3 threads for production software.
 * 
 * There are a number of "events" that a listener profile
 * can implement. Some MUST be implemented.
 *
 * - OnFramRecv [MANDATORY]
 *   Called whenever a complete frame is received.
 * - OnChanCreate
 * - OnChanPeerCloseRequest
 * - OnChanClosed
 * 
 * All of these methods receive a pointer to the current
 * channel object. No other data is passed to the caller.
 * The channel object, however, contains a pointer to the
 * current profile object, where the caller in turn can
 * include a pointer to more relevant data.
 *
 * Please note that there is no "peer connect" event. The
 * peer will never connect to a profile, but to a session.
 * The session (channel 0) is then used to open up a channel.
 * Consequently, the OnChanCreate() event is the first event
 * that a profile can receive.
 *
 * The profile is responsible for following proper order
 * of messages. We think that a profile will most probably
 * be implemented as a state machine, but this is left to the
 * profile implementor.
 *
 * A profile MUST support the following callbacks:
 * - SendSEQUpTo
 *   This is used by the BEEP stack to ask the profile up
 *   to which message ID a SEQ frame should be sent. The
 *   profile must return the ID of the last frame that
 *   it has processed or queued (at the choice of the profile).
 *
 * As a hint to profile developers, SendSEQUpTo() can be used to
 * allow an in-memory queue to grow up to a specifc size and
 * if it outgrows this size to slow down the receiver. If a
 * profile is implemented in that way, it allows for fast buffering
 * of burst traffic but prevents exhaustion of system ressources
 * when too much data comes in too quickly. As a general
 * guideline, we recommend that a profile initially buffers
 * incoming data and acknowledges all received packets 
 * immediately (when the BEEP stack calls SendSEQUpTo()).
 * If the queue threshold is reached, it then should always
 * return the same ID it returned previously. That way, no
 * new SEQ will be sent to the remote peer and the remote
 * peer will stop sending new data once the window size is
 * reached. Please note that the profile will probably receive
 * multiple  calls to SendSEQUpTo() during this process as
 * the remote peer will continue to send data until the end
 * of the window. As such, some more message will need to be
 * enqueued and OnMesgReceive() will be called some more times.
 * Once the queue has been emptied beyond the threshold, the
 * client can again answer with the most recent ID, which
 * will make the BEEP stack send a SEQ frame for that ID and
 * the remote peer will continue to send data. Now the same
 * process begins again, eventually the server will again hit
 * the threshold, stop acknowledging messages and thus slowing
 * down the sender. If using the logic described here, the
 * queue will never grow above queue threshold + size of window.
 *
 * Of course, the above described is only a recommendation to
 * the profile implementor and the profile may acknowledge 
 * each message immediately, not paying attention to any queue.
 * Or it may defer acknowledging the entry until some action has 
 * been carried out. Whatever it does, it must make sure that
 * it suffciently often acknowledges the messages as when the
 * sender hits the window size AND does not receive an ACK,
 * the sender will not send any more packets. Such a connection
 * would be stalled and no way to recover it. It is the profile
 * implementor's task to avoid this. Nobody will save you from
 * this ;)
 *
 * Of course, the above description also includes much information
 * on how the BEEP stack should operate. If we look into the
 * specifics there, we see there is need for SendSEQUpTo() to
 * be called frequently. At least, it must be called once AFTER
 * each OnMesgRecv(). If the caller always immediately acknowledges
 * the msgno, things are straightforward: we can send a SEQ
 * frame and communication can carry on. However, if the caller
 * defers acknowledgment, we may run into an issue. Again,
 * everything is easy if the caller acknowledges the msgno
 * before the end of the window is reached - a SEQ frame will 
 * be sent and communication continues. However, if the caller
 * is not ready to acknoledge the msgno after the last OnMesgRecv()
 * in this window, we need to ask him at other occasions for the
 * acknowledgement. The reason is that OnMesgRecv() will never
 * be called once the window has been used up - no new messages
 * can arrive in this stage. We assume, this situation will
 * often occur during traffic bursts (assuming a "good" profile
 * implementation). So we must handle this efficiently.
 *
 * A key point is that we must try conserve system ressources
 * while on the other hand providing a fast response time
 * once the profile is ready to accept new data. As we need to
 * call the profile, there is some polling involved. We have
 * several ways to talk to the profile.
 *
 * First of all, we can use our sbLstnRun() method
 * to call back to any "stalled" profiles as soon as there is
 * ANY network activity (meaning new connections or frames 
 * received - could even include frames send from another
 * profile/channel). This is a natural way, as the method
 * already was awoken and there is little overhead in doing
 * the SendSEQUpTo(). 
 * 
 * Unfortunately, this works only well if we have frequent
 * other traffic. In a scenario with just one channel active,
 * that doesn't help anything at all - there is no network
 * activity. In this case, we need to work with timeouts
 * (driving an actual polling cycle). The problem with a timeout
 * obviously is that if we use a too short period, we eat up
 * too many system ressources and using a too long period, we
 * loose performance and response time. Past experience shows
 * that a fixed timeout period can most probably be only set
 * inappropriately ;)
 *
 * We are trying to avoid this with the following algorithm:
 * The timeout period is dynamically adjusted. We assume
 * that the longer an acknowledgement is outstanding, the less
 * likely is a quick acknowledgment. We also assume that even
 * when an acknowledgment would happen very quickly (but after
 * an overall long wait period), it would not really matter 
 * if we have some more delay in sending the SEQ because
 * the work queue for the profile would probably still being
 * very full (given a "good" profile implementation according
 * to our recommendation). We take advantage of this approach.
 * The timeout period is initialized with a 10ms timeout. If
 * it expires, SendSEQUpTo() is called for all profiles with 
 * missing acknowledgments. If none of them returns a new
 * acknowledgment, the timeout value is doubled. Then a new
 * wait is done for at most this timeout period. After it returns,
 * SendSEQUpTo() is called again ... and so on. The timeout
 * value is always double until it reaches a maxmimum timout
 * value, which could be 640 ms or such. The initial and maximum
 * timeout values could be configurable parameters. Once the
 * maximum timeout value is reached, processing continues as
 * described, only the timeout value is never increased.
 * If any of the profiles returns a new acknowledgement, the
 * corresponding SEQ is sent and the algorithm is completely
 * restarted again. Restarted means it begins to look if there
 * are outstanding acknowledgments and then it decides
 * wheter to use timeout or not - and if so, it starts from
 * the initial timeout value.
 *
 * Note well: this algorithm is ONLY applied if we have
 * at least one channel who is "stalled" in a way that
 * the profile must acknowledge outstanding msgnos. If all
 * profiles/channels have acknowledge the last received
 * message, the algorithm is NOT used and a "normal"
 * blocking wait for network activity is done. The author
 * assumes that this is the normal case and as such
 * any impact from a still-improper timeout value is
 * minimalistic.
 *
 *

 *
 * \subsection proglang Progamming Language Used
 * While it is tempting to use C++, we have decided to
 * stick with C in order to make the lib available to
 * as many platforms as possible.
 *
 * \subsection objects Objects
 *
 * WARNING: this section is not totally up to date!
 *
 * We use an object-oriented approach even though
 * the lib is written in plain C. Objects are
 * simulated by instance structures. All non-static
 * class members receive a "this" point (pointer to
 * the instance data structure) passed to as the first
 * parameter. The constructor will create the instance
 * data structures.
 *
 * The following classes are known as of now:
 * - \class sbSock sockets.h
 * - BEEP "pretender classes":
 *   - \class sbSess beepsession.h
 *   - \class sbChan beepchannel.h
 *   - \class sbFram beepframe.h
 *   - \class sbMesg beepmessage.h
 *   - \class sbProf beepprofile.h
 * - \class srAPI srAPI.h
 *
 * Each class is contained in the respective source file.
 * Class names either start with sr (syslog reliable) or
 * sb (slim beep). They are followed by a 4-character
 * abbreviation. This serves also as a namespace - the
 * 6 character class name is prepended to all method 
 * names.
 *
 * \section naming Naming Conventions
 * As this is plain C, we do not have a real namespace. To avoid
 * conflicts, all globals start with prefixes, followed by
 * the actual function name. The function/method/data object
 * name begin with a capital letter. The prefix is
 * mentioned in the respective header.
 *
 * \section authors Authors
 * This project was initiated by Rainer Gerhards, who is also
 * the developer of the initial version. Rainer can be
 * contacted at rgerhards@adiscon.com (Watch for the 
 * Anti-Spam reply and follow the instructions!)
 *  
 * \section restrictions Restrictions
 * - tuning profiles are not supported
 * - only 2 channels (channel 0 and 1 syslog channel) are
 *   supported
 * - at this time, only the RAW profile is supported
 * 
 * The file architecture.c does NOT contain any code - it
 * is solely used to provide this documentation.
 *
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1.0 Initial version created
 *
 * \date    2003-08-05
 *          0.1.1 version - now compiles under Linux
 *          (confirmed with Red Hat 8.0). There is
 *          also a makefile who does the trick.
 *          This is an interim release - it already
 *          includes some linked list code, which so far
 *          is used nowhere. This is in preparation of
 *          the 0.2.0 release which will include an
 *          XML parser and more robust BEEP protocol
 *          error handling.
 *
 *  \date	2003-08-13
 *			Description for listener operations added
 *			(begins 0.3.0, what probably later will
 *          become 1.0.0).
 *
 * \section legal Legal State
 *
 * Copyright 2002-2014 
 *     Rainer Gerhards and Adiscon GmbH. All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
