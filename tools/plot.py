#!/usr/bin/env python3

# FlowEmu - Flow-Based Network Emulator
# Copyright (c) 2021 Institute of Communication Networks (ComNets),
#                    Hamburg University of Technology (TUHH),
#                    https://www.tuhh.de/comnets
# Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import sys
import re
import matplotlib.pyplot as plt

# Get path to log file
log_path = sys.argv[1]

# Create data arrays
x = []
y = []

# Read data from log file
with open(log_path, mode="r") as log_file:
	for line in log_file.readlines():
		# Get queue length of FIFO queue
		m = re.search("\[([\d\.]+)\] get/module/fifo_queue/queue_length ([\d\.]+)", line)
		if m:
			x.append(float(m.group(1)) / 1000)
			y.append(float(m.group(2)))
			continue

# Subtract timestamp of first data point from all data points
start_timestamp = x[0]
x = [i - start_timestamp for i in x]

# Plot data
plt.plot(x, y)

# Label axis
plt.title("FIFO Queue")
plt.xlabel("Time [s]")
plt.ylabel("Queue Length [packets]")

# Show plot
plt.tight_layout()
plt.show()
