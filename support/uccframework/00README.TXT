This module implements the Ensigma common system framework for UCCP applications. Actually this
is a rudimentary stub for a more sophisticated framework, yet to be developed.

Apart from the system-level sizing constants (see "configuration" below), it is not intended
that this module is changed in the course of a TV standard development - indeed in a multi-standard
system, this module will be shared by all the standards, so standard-specific changes must be avoided.

Any changes that *are* necessary - be they due to bugs or feature changes, MUST
be coordinated with the Ensigma tools group, who retain responsibility for this
module.

For now, each TV standard developer is expected to work with a *copy* of this module,
which is configured at compile time by adjusting the sizing constants in
an application-specific copy of the configuration header file

Eventually, we expect that this framework will be distributed within a TV support
library, with a run-time configuration mechanism.

Configuration
=============

An example configuration file is provided called ufwconfig_example.h. The actual configuration
file used by the software is called ufwconfig.h. Each application should take a renamed copy of
ufwconfig_example.h, which can then be edited as required.

In a "smake" build environment, the normal arrangement is for each application to create a MOD
called ufwconfig which has only one source file - ufwconfig.h. This approach is illustrated by
the applications included in the application shell distribution.
