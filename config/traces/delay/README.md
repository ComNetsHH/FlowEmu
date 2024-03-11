Please copy your delay traces to this directory!

The trace format is compatible with the traces for [Link'Em](https://github.com/sys-uos/linkem).

Therefore, the files contain a sequence of integers separated by spaces. Each entry represents one packet. The value gives the delay that should be added to the packet in ms. Entries from multiple lines are concatenated. A space after the last element is not required. If the end of the trace is reached, it starts over from the beginning.
