Please copy your loss traces to this directory!

The trace format is compatible with the traces for [Link'Em](https://github.com/sys-uos/linkem).

Therefore, the files contain a sequence of 0 and 1. Each entry represents one packet. 0 means packet loss and 1 means no packet loss. Entries from multiple lines are concatenated and all other characters are ignored. If the end of the trace is reached, it starts over from the beginning.
