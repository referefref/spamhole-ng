# spamhole-ng
A fork of Spamhole by Dustin Trammell with added safety

## Author's original notes
			SpamHole Doc
		  http://www.spamhole.net

```I)ruid [CAU]					druid@caughq.org```

----
### Theory & Method:

Many spammers constantly search for open SMTP relays, or mail
servers that will allow anyone to send e-mail through them.
Using these servers, they send mass amounts of unsolicited bulk
mail (SPAM).  Most diligent mail admins on the Internet these
days do not run open relays, however there are many out there
that still do.  Many viruses and trojans are beginning to appear
that turn an unsuspecting cablemodem or DSL user's computer
into an open relay without the user's knowledge.  The most common
way for spammers to find these open relays is for the virus or
trojan to report it's existance directly to them, or by
methodically scanning netowrks looking for them.  This project
hopes to make the latter method a waste of time and effort.

To accomplish our goal, we take the chaff approach.  By creating
as many false 'open relays' on the Internet as possible, we hope
to make the detection and use of a real open relay as much of a
chore as we can, as well as waste as much time and effort as we
can of any spammers that find and use our spamholes as if they
were real open relays.  To accomplish this, we take a rather 
simple approach:

When an SMTP client connects to our spamhole, the spamhole will
emulate an SMTP open relay, happily accepting any email messages
that the client wishes to send to it, however rather than
actually delivering the messages, it will silently drop them.


----
### Notes:

This is a REFERENCE IMPLEMENTATION.  You should compile and
run at YOUR OWN RISK.  This code is most likely riddled with
bugs. and is meant to provide a proof-of-concept only.
You are highly encouraged to develop your own spamhole code and 
share it with others through this project.  The more platforms 
supported, the more chaff the spammers will have to deal with to 
find the REAL open SMTP relays.


----
### License:

spamhole is released under the GNU General Public License (GPL).
You can find more information on the GPL here:

http://www.gnu.org/copyleft/gpl.html


----
To install and run:

1. Check your config options at the top of config.h

2. type 'make'

3. Copy spamhole to wherever you would like it to live.

4. Execute spamhole as root, it will drop privs to the UID you 
defined in spamhole.h after binding to it's port (usually 25)


----
spamhole.h Config Options:

UID - spamhole's running UID (after binding to the port, it
	drops privs to this user)

LOCAL_IP - The local address you want spamhole to listen on (use
	"0.0.0.0" for all local addresses).

LOCAL_PORT - The local port for spamhole to listen to (usually
	port 25)
