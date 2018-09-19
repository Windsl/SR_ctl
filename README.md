# SR_ctl
Distributed control system framework for robotics

SR_ctl is framework for distributed control system for robotics.

The framework allow to assemble control systems from many end devices are joined by Ethernet network.
The control system is based on Message Passing Interface (MPI).
Control system algorithms are realized as .so files that compiled from C++ file produced by special template.

Different algorithm .so files are linked by variables transmission. Each algorithm has inputs and outputs.
Output value form one algorithm transmits to input of another algorithm if names of the inpot and output correspond one to each other.

Algorithms work with constant period on each device. Algorithms work one after other.
